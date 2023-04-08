#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <tuple>

//
#define MINIAUDIO_IMPLEMENTATION
#include "sound_engine.hpp"
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
	cliph::utils::raw_audio_buf* capt_buf{nullptr};
	cliph::utils::raw_audio_buf* playb_buf{nullptr};
	std::chrono::microseconds callb_period{0};
};
user_data u_d;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	user_data& u_d = *static_cast<user_data*>(pDevice->pUserData);
	auto& cptb = *u_d.capt_buf;
	auto& plbb = *u_d.playb_buf;
	const auto& total_time_budget = u_d.callb_period;
	assert(u_d.capt_buf);
	assert(u_d.playb_buf);
	assert(total_time_budget != std::chrono::microseconds{});

	// Get samples from device
	const auto start = std::chrono::steady_clock::now();
	const auto capt_buf_put_time_budget = total_time_budget/2; // TODO: improve ???
	const auto len = frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
	if (!cptb.put_for(pInput, len, capt_buf_put_time_budget))
	{
		std::cerr << "Raw audio capture buffer overrun\n";
	}

	// Put samples to device
	const auto time_budget_left = std::chrono::steady_clock::now() - start;
	auto raw_audio_buf = std::array<uint8_t, 4096u>{};
	if (auto sz = plbb.get_for(raw_audio_buf.data(), raw_audio_buf.size(), time_budget_left))
	{
	    MA_COPY_MEMORY(pOutput, raw_audio_buf.data(), *sz * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
	}
	else
	{
		std::cerr << "Raw audio playback buffer underrun\n";
	}
}	
	
#if 0

	//
	if (!u_d.capt_buf->put(pInput, frameCount))
	{
		std::cerr << "Audio buffer overrun" << '\n';
	}

	auto chunk = cliph::utils::th_safe_circ_buf<int16_t>::chunk_type{};
	auto* buf = std::get<0>(chunk).data();
	if (auto frame_sz = u_d.playb_buf->get(buf))
	{
		MA_COPY_MEMORY(pOutput, buf, frame_sz*ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
	}
}

	//
	rtp_stream.advance_ts(user_data::k_opus_dyn_pt, user_data::k_rtp_advance_interval);
	auto* opus_data_start = static_cast<unsigned char*>(rtp_stream.fill(pkt_buff.data(), pkt_buff.size()));

	const auto buf_len_remains = pkt_buff.size() - (opus_data_start - pkt_buff.data());
    //
    if (auto encoded = ::opus_encode(opus_enc, static_cast<const opus_int16*>(pInput), frameCount, opus_data_start, buf_len_remains); encoded < 0)
    {
        std::cerr << get_opus_err_str(encoded) << '\n';
        return;
    }
    else
    {
		static auto talkspurt_start = true;
		if (encoded > 2) // not dtx
		{
			rtp_stream.advance_seq_num();
			if (talkspurt_start)
			{
				rtpp::rtp{pkt_buff.data(), pkt_buff.size()}.mark();
				talkspurt_start = false;
			}
			const auto total_pkt_len = opus_data_start - pkt_buff.data() + encoded;
			sock.write(u_d.remote(), pkt_buff.data(), total_pkt_len);
	    }
		else
	    {
			talkspurt_start = true;
	    }
    }

    // try playout
	auto decode_buf = std::array<opus_int16, 2048>{};
	{
		if (recv_buf.m_is_empty)
		{
			std::cerr << "Audio buffer underrun" << '\n';
			return;
		}
#if 0
		const auto rtp_pkt = cliph::rtp::rtp{recv_buf.m_buffer.data(), recv_buf.m_size};
		assert(rtp_pkt);
		std::cerr << rtp_pkt << '\n';
#endif
		// we only offer OPUS and nothing else, so only expect OPUS payloads
		// TODO: incoming pt check
		const auto rtp_pkt = rtpp::rtp{recv_buf.m_buffer.data(), recv_buf.m_size};
		if (rtp_pkt.pt() != u_d.m_rmt_opus_pt)
		{
			std::cerr << "Got unexpected pt: " <<  rtp_pkt.pt() << '\n';
			return;
		}
		if (auto frame_sz = ::opus_decode(opus_dec, rtp_pkt, rtp_pkt.size(), decode_buf.data(), decode_buf.size(), /*decode_fec*/ 0); frame_sz < 0)
	   	{
	        std::cerr << get_opus_err_str(frame_sz) << '\n';
	   	}
		else
	   	{
            MA_COPY_MEMORY(pOutput, decode_buf.data(), frame_sz * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
	   	}
	}
}
#endif

} //anon namespace


namespace cliph::sound
{

engine::engine(const cliph::sound::config& cfg
	, utils::raw_audio_buf& capt_buf
	, utils::raw_audio_buf& playb_buf)
	: m_cfg{cfg}
	, m_cpt_buf{capt_buf}
	, m_plb_buf{playb_buf}
{
	if (ma_context_init(NULL, 0, NULL, &m_context) != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to initialize context"};
	}

	u_d.capt_buf = std::addressof(m_cpt_buf);
	u_d.playb_buf = std::addressof(m_plb_buf);
	u_d.callb_period = m_cfg.m_period_sz;

	enumerate_and_select();
}


engine::~engine()
{
    ma_device_uninit(&m_device);
    ma_encoder_uninit(&m_encoder);
    ma_context_uninit(&m_context);
}

void engine::enumerate_and_select()
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

void engine::run()
{
	if (auto result = ma_device_start(&m_device); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to start device"};
	}
}

void engine::pause()
{
	if (auto result = ma_device_stop(&m_device); result != MA_SUCCESS)
	{
		throw std::runtime_error{"Failed to stop device"};
	}
}

void engine::stop()
{
	pause();
	u_d.capt_buf->put(nullptr, 0);
}

} // namespace cliph::audio
