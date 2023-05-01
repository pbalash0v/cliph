#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <tuple>

//
#define MINIAUDIO_IMPLEMENTATION
#include "sound_device.hpp"
//

using namespace std::chrono_literals;

namespace  
{

auto enumerate_pb(ma_context& context)
{
	ma_device_info* pPlaybackDeviceInfos{};
	ma_uint32 playbackDeviceCount{};
	ma_uint32 captureDeviceCount{};
	ma_device_info* pCaptureDeviceInfos{};
	ma_uint32 iDevice{};

    if (auto result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to retrieve device information"};
	}

	std::printf("\nPlayback Devices\n");
	for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice)
	{
		std::printf("    %u: %s\n", iDevice, pPlaybackDeviceInfos[iDevice].name);
	}

    return std::tuple{playbackDeviceCount-1, pPlaybackDeviceInfos};
}

auto enumerate_cap(ma_context& context)
{
	ma_device_info* pPlaybackDeviceInfos{};
	ma_uint32 playbackDeviceCount{};
	ma_uint32 captureDeviceCount{};
	ma_device_info* pCaptureDeviceInfos{};
	ma_uint32 iDevice{};

    if (auto result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to retrieve device information"};
	}

	std::printf("\nCapture Devices\n");
	for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice)
	{
		std::printf("    %u: %s\n", iDevice, pCaptureDeviceInfos[iDevice].name);
	}

    return std::tuple{captureDeviceCount-1, pCaptureDeviceInfos};
}

//
struct user_data
{
	cliph::data::media_queue* cpt_q{nullptr};
	cliph::data::media_stream* cpt_strm{nullptr};
	//
	cliph::data::media_queue* ply_q{nullptr};
	cliph::data::media_stream* ply_strm{nullptr};
	//
	std::chrono::milliseconds callb_period{0};
	unsigned sample_rate{};
};
user_data u_d;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	user_data& u_d = *static_cast<user_data*>(pDevice->pUserData);
	assert(u_d.cpt_q); assert(u_d.ply_q);
	auto& cpt_q = *u_d.cpt_q;
	auto& cpt_strm = *u_d.cpt_strm;
	auto& ply_q = *u_d.ply_q;
	auto& ply_strm = *u_d.ply_strm;

	//
	// Get samples from device
	//
	if (auto slot = cpt_q.get())
	{
		auto& media = slot->m_sound;
		const auto bytes = frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
		MA_COPY_MEMORY(media.data.data(), pInput, bytes);
		media.sz = u_d.callb_period;
		media.sample_count = frameCount;
		media.bytes_count = bytes;
		media.sample_rate = u_d.sample_rate;
		if (not cpt_strm.try_put(std::move(slot)))
		{
			std::cerr << "Slot ring capture buffer overrun\n";
		}
	}
	else
	{
		std::cerr << "Raw audio capture buffer overrun\n";
	}

	//
	// Put samples to device
	//
	using slot_type = typename std::remove_reference_t<decltype(ply_q)>::slot_type;
	auto slot = slot_type{};
	if (ply_strm.try_get(slot))
	{
		if (
			(slot->m_sound.sample_rate != 48000u)
			or (slot->m_sound.sample_count == 0)
			or (slot->m_sound.sz != u_d.callb_period)
			)
		{
			std::cerr << "Unhandled media element, playback failed" << '\n';
			return;
		}

		const auto bytes = slot->m_sound.sample_count * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
	    MA_COPY_MEMORY(pOutput, slot->m_sound.data.data(), bytes);
	    slot->reset();
	}
	else
	{
		std::cerr << "Raw audio playback buffer underrun\n";
	}
}
} //anon namespace


namespace cliph::sound
{

device::device(const cliph::sound::config& cfg
	, data::media_queue& capt_q
	, data::media_stream& capt_strm
	, data::media_queue& playb_q
	, data::media_stream& playb_strm)
	: m_cfg{cfg}
	, m_cpt_q{capt_q}
	, m_cpt_strm{capt_strm}
	, m_plb_q{playb_q}
	, m_plb_strm{playb_strm}
{
	if (ma_context_init(NULL, 0, NULL, &m_context) != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to initialize context"};
	}

	u_d.cpt_q = std::addressof(m_cpt_q);
	u_d.cpt_strm = std::addressof(m_cpt_strm);
	u_d.ply_q = std::addressof(m_plb_q);
	u_d.ply_strm = std::addressof(m_plb_strm);
	u_d.callb_period = m_cfg.m_period_sz;
	u_d.sample_rate = m_cfg.m_audio_device_sample_rate;

	enumerate_and_select();
}


device::~device()
{
    ma_device_uninit(&m_device);
    ma_encoder_uninit(&m_encoder);
    ma_context_uninit(&m_context);
}

void device::enumerate_and_select()
{
    ///
    const auto dev_id_get = [](auto max_dev_id, auto* device_infos)
    {
        while (true)
        {
            auto dev_id = 0u;
            std::cin >> dev_id;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (dev_id >= 0 and dev_id <= max_dev_id)
            {
                std::printf("%u: %s selected\n", dev_id, device_infos[dev_id].name);
                return dev_id;
            }
            else
            {
                std::printf("%u: wrong selection\n", dev_id);
            }
        }
    };
	auto [max_pb_dev_id, pPlaybackDeviceInfos] = enumerate_pb(m_context);
	//
    std::printf("Select playback device:\n");
    const auto pb_dev_id = dev_id_get(max_pb_dev_id, pPlaybackDeviceInfos);
    auto [max_cap_dev_id, pCaptureDeviceInfos] = enumerate_cap(m_context);
    std::printf("Select capture device:\n");
    const auto cap_dev_id = dev_id_get(max_cap_dev_id, pCaptureDeviceInfos);
    //
    constexpr const auto kPeriodSizeInFrames = 0u;
    auto deviceConfig = ma_device_config_init(ma_device_type_duplex);
    // pback props
    deviceConfig.playback.pDeviceID = &pPlaybackDeviceInfos[pb_dev_id].id;
    deviceConfig.playback.format   = ma_format_s16;
    deviceConfig.playback.channels = m_cfg.m_audio_device_stereo_playback ? 2 : 1;
	// cap props
    deviceConfig.capture.pDeviceID = &pCaptureDeviceInfos[cap_dev_id].id;
    deviceConfig.capture.format   = ma_format_s16;
    deviceConfig.capture.channels = m_cfg.m_audio_device_stereo_capture ? 2 : 1;
    // common
    deviceConfig.sampleRate       = m_cfg.m_audio_device_sample_rate;
    deviceConfig.periodSizeInFrames = kPeriodSizeInFrames;
    deviceConfig.periodSizeInMilliseconds = m_cfg.m_period_sz.count();
    deviceConfig.dataCallback     = data_callback;
    deviceConfig.pUserData        = &u_d;

	if (auto result = ma_device_init(NULL, &deviceConfig, &m_device); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to initialize capture device"};
	}

#if 0
    auto encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, k_channel_count, k_sample_rate);

    ma_encoder encoder;
    
    if (ma_encoder_init_file(argv[1], &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize output file.\n");
        return -1;
    }
#endif
}

void device::run()
{
	if (auto result = ma_device_start(&m_device); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to start device"};
	}
}

void device::pause()
{
	if (auto result = ma_device_stop(&m_device); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to stop device"};
	}
}

void device::stop()
{
	pause();
	for (;;)
	{
		if (auto slot = m_cpt_q.get())
		{
			slot->reset();
			m_cpt_strm.put(std::move(slot));
			break;
		}
	}
}

} // namespace cliph::audio
