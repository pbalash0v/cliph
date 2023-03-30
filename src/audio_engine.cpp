#include "asio/ip/address.hpp"
#include "asio/ip/address_v4.hpp"
#include "asio/ip/udp.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <tuple>

//
#include <opus.h>
//
#define MINIAUDIO_IMPLEMENTATION
#include "audio_engine.hpp"
#include "rtp_stream.hpp"
#include "rtp.hpp"
#include "net.hpp"
#include "sdp.hpp"
#include "utils.hpp"

namespace
{
constexpr const auto k_audio_device_sample_rate = 48000u;
constexpr const auto k_audio_device_channel_count = 1;
} //namespace

namespace
{

auto* get_opus_enc()
{
    int error{};
    auto* e = opus_encoder_create(k_audio_device_sample_rate, k_audio_device_channel_count, OPUS_APPLICATION_VOIP, &error);
    
    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

    if (auto ret = opus_encoder_ctl(e, OPUS_SET_DTX(1)); ret != OPUS_OK)
    {
       throw std::runtime_error{"OPUS_SET_DTX failed"};
    }

    return e;
}

auto* get_opus_dec()
{
    int error{};
    auto* e = opus_decoder_create(k_audio_device_sample_rate, k_audio_device_channel_count, &error);
    
    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

    return e;
}

std::string_view get_opus_err_str(opus_int32 err)
{
    switch (err)
    {
    case OPUS_OK: return "OPUS_OK";
    case OPUS_BAD_ARG: return "OPUS_BAD_ARG";        
    case OPUS_BUFFER_TOO_SMALL: return "OPUS_BUFFER_TOO_SMALL";        
    case OPUS_INTERNAL_ERROR: return "OPUS_INTERNAL_ERROR";
    case OPUS_INVALID_PACKET: return "OPUS_INVALID_PACKET";
    case OPUS_UNIMPLEMENTED: return "OPUS_UNIMPLEMENTED";
    case OPUS_INVALID_STATE: return "OPUS_INVALID_STATE";
    case OPUS_ALLOC_FAIL: return "OPUS_ALLOC_FAIL";
    default: return "OPUS_UNKNOWN";
    }
}

} // anon namespace



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

class user_data final
{
public:
	inline static constexpr const auto kPeriodSizeInMilliseconds = 20u;
	inline static constexpr const auto k_rtp_advance_interval= rtpp::stream::duration_type{kPeriodSizeInMilliseconds};
	inline static constexpr const auto k_opus_dyn_pt = 96u;
	inline static constexpr const auto k_opus_rtp_clock = 48000u;
public:
	OpusEncoder* opus_enc_ctx{};
	OpusDecoder* opus_dec_ctx{};
	//
	rtpp::stream rtp_stream;
	//
	std::unique_ptr<cliph::net::socket> sock_ptr;
	//
	asio::ip::address local_media;
	//
	std::atomic<std::uint8_t> m_rmt_opus_pt{};
	//
	std::array<unsigned char, 4096> buff;
	cliph::utils::thread_safe_array recv_buf;

	auto remote()
	{
		auto lock_version = std::uint64_t{};

		while (true)
		{
			while (true)
			{
				lock_version = m_rmt_seq_lock_version.load();
				if ((lock_version & 1) != 0) continue; //write in progress
				break;
			}

			auto res = m_remote;
			if (m_rmt_seq_lock_version == lock_version)
			{
				return res;
			}
		}
	}

	auto remote(asio::ip::udp::endpoint rmt)
	{
		m_rmt_seq_lock_version += 1;
		m_remote = std::move(rmt);
		m_rmt_seq_lock_version += 1;
	}

private:
	//
	asio::ip::udp::endpoint m_remote;

	std::atomic<std::uint64_t> m_rmt_seq_lock_version{};
};
auto u_d = user_data{};

//
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	user_data& u_d = *static_cast<user_data*>(pDevice->pUserData);
	auto* opus_enc = u_d.opus_enc_ctx;
	auto* opus_dec = u_d.opus_dec_ctx;
	auto& pkt_buff = u_d.buff;
	auto& rtp_stream = u_d.rtp_stream;
	auto& sock = *(u_d.sock_ptr);
	auto& recv_buf = u_d.recv_buf;
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
		auto lock = std::lock_guard{recv_buf};
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
		recv_buf.mark_empty_locked();
	}
	recv_buf.m_cond.notify_one();
}

} //anon namespace


namespace cliph
{

engine& engine::get() noexcept
{
	static auto instance = engine{};
	return instance;
}

engine& engine::init(const config& cfg)
{
    if (ma_context_init(NULL, 0, NULL, &m_context) != MA_SUCCESS)
    {
        throw std::runtime_error{"Failed to initialize context"};
    }

	u_d.opus_enc_ctx = get_opus_enc();
	u_d.opus_dec_ctx = get_opus_dec();
	//

	u_d.rtp_stream = rtpp::stream{};
	u_d.rtp_stream.payloads().emplace(user_data::k_opus_dyn_pt, user_data::k_opus_rtp_clock);
	//
	u_d.local_media = cfg.net_iface;
	u_d.sock_ptr = std::make_unique<cliph::net::socket>(cfg.net_iface, u_d.recv_buf);
	u_d.sock_ptr->run();
	//
	enumerate_and_select();

	return *this;
}

void engine::set_net_sink(std::string_view ip, std::uint16_t port)
{
	u_d.remote(asio::ip::udp::endpoint{asio::ip::make_address_v4(ip), port});
}

void engine::set_remote_opus_params(std::uint8_t pt)
{
	u_d.m_rmt_opus_pt = pt;
}


std::string engine::description() const
{
	return cliph::sdp::get_local(u_d.local_media.to_string(), u_d.sock_ptr->port()).c_str();
}

engine::~engine()
{
	u_d.sock_ptr->stop();
	::opus_encoder_destroy(u_d.opus_enc_ctx);
	::opus_decoder_destroy(u_d.opus_dec_ctx);
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
    deviceConfig.playback.channels = k_audio_device_channel_count;
	// cap props
    deviceConfig.capture.pDeviceID = &pCaptureDeviceInfos[cap_dev_id].id;
    deviceConfig.capture.format   = ma_format_s16;
    deviceConfig.capture.channels = k_audio_device_channel_count;
    // common
    deviceConfig.sampleRate       = k_audio_device_sample_rate;
    deviceConfig.periodSizeInFrames = kPeriodSizeInFrames;
    deviceConfig.periodSizeInMilliseconds = user_data::kPeriodSizeInMilliseconds;
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

void engine::start()
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
	{
		auto lock = std::lock_guard{u_d.recv_buf};
		u_d.recv_buf.mark_empty_locked();
	}
    u_d.sock_ptr->stop();
}

} // namespace cliph::audio
