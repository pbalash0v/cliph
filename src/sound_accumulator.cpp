#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ratio>
#include <thread>
//
#include <rtp.hpp>
#include "sound_accumulator.hpp"
#include "controller.hpp"

//

using namespace std::chrono;
using namespace std::chrono_literals;

namespace
{

auto* get_opus_enc(std::uint32_t sample_rate, uint8_t channel_count, bool dtx)
{
	auto error = int{};
    auto* e = opus_encoder_create(sample_rate, channel_count, OPUS_APPLICATION_VOIP, &error);

	if (error != OPUS_OK)
	{
		throw std::runtime_error{"opus_encoder_create failed"};
	}

    if (dtx)
    {
		if (auto ret = opus_encoder_ctl(e, OPUS_SET_DTX(1u)); ret != OPUS_OK)
		{
			throw std::runtime_error{"OPUS_SET_DTX failed"};
		}
	}

	return e;
}

auto* get_opus_dec(std::uint32_t sample_rate, uint8_t channel_count)
{
	auto error = int{};
	auto* d = opus_decoder_create(sample_rate, channel_count, &error);

    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

	return d;
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

namespace cliph::sound
{

accumulator::accumulator(cliph::controller& controller
	, const cliph::config& cfg
	, data::raw_audio_buf& capt_buf
	, data::raw_audio_buf& playb_buf
	, data::media_queue& audio_q
	, data::media_buf& audio_buf)
	: m_controller{controller}
	, m_snd_cfg{cfg.m_snd}
	, m_capt_snd_cbuf{capt_buf}
	, m_playb_snd_cbuf{playb_buf}
	, m_audio_q{audio_q}
	, m_audio_buf{audio_buf}
	, m_socket{asio::ip::udp::socket{m_io, asio::ip::udp::endpoint{cfg.local_media_ip, 0u}}}
{
	m_opus_enc_ctx = get_opus_enc(m_snd_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u, /*dtx */true);
	m_opus_dec_ctx = get_opus_dec(m_snd_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u);

	m_strm.payloads().emplace(96,rtpp::payload_type{48000u});
}

accumulator::~accumulator()	
{
	if (m_egress_thr.joinable()) { m_egress_thr.join(); }
	if (m_ingress_thr.joinable()) { m_ingress_thr.join(); }

	::opus_encoder_destroy(m_opus_enc_ctx);
	::opus_decoder_destroy(m_opus_dec_ctx);
}

void accumulator::run()
{
	m_egress_thr = std::thread{[this]()
	{
		std::cerr << "accumulator egress_loop started\n";
		egress_loop();
		std::cerr << "accumulator egress_loop finished\n";
	}};

	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void accumulator::egress_loop()
{
	// To encode a frame opus_encode() must be called with exactly one frame (2.5, 5, 10, 20, 40 or 60 ms) of audio data
	const auto k_lengths = std::array<milliseconds, 4u>{10ms, 20ms, 40ms, 60ms};
	auto media = data::media{};
	bool is_dtx = false;
	const auto reset = [&]
	{
			media.reset();
			m_egress_snd_offset = 0;
			m_next_len = k_lengths[std::rand() % k_lengths.size()];
	};

	while (true)
	{
		if (const auto sz = m_capt_snd_cbuf.get(m_egress_snd_accum.data() + m_egress_snd_offset, m_egress_snd_accum.size()))
		{
			m_egress_snd_offset += sz;
			//
			const auto samples_per_ms = m_snd_cfg.m_audio_device_sample_rate/1000u;
			const auto curr_accum = milliseconds{(m_egress_snd_offset/sizeof(int16_t))/samples_per_ms};
			assert(milliseconds{(sz/sizeof(int16_t))/samples_per_ms} == 10ms);
			if (curr_accum < m_next_len) { continue; }

			// encode
			if (const auto encoded = ::opus_encode(m_opus_enc_ctx
				, m_egress_snd_accum.data(), m_egress_snd_offset
				, media.rtp_data.data(), media.rtp_data.size())
				; encoded < 0)
			{
				std::cerr << get_opus_err_str(encoded) << '\n';
			}
			else
			{
				if (encoded < 3) // DTX
				{
					std::printf("DTX\n");
					is_dtx = true;
				}
				else
				{
					is_dtx = false;
					media.rtp_data_sz = encoded;
					std::printf("%ld ms, encoded: %d\n", curr_accum.count(), encoded);
				}
			}

			// packetize
			m_strm.advance_ts(96, curr_accum);
			if (is_dtx)
			{
				reset();
				continue;
			}
			m_strm.advance_seq_num();

			const auto hdr_sz = static_cast<unsigned char*>(m_strm.fill(media.rtp_hdr.data(), media.rtp_hdr.size())) - media.rtp_hdr.data();
			std::cerr << rtpp::rtp{media.rtp_hdr.data(), static_cast<size_t>(hdr_sz)} << ", hdr_sz: " <<  hdr_sz << '\n';

			// send
			const auto* encoded = media.rtp_data.data();
			const auto enc_len = media.rtp_data_sz;
			const auto* rtp_hdr = media.rtp_hdr.data();
			const auto rtp_hdr_len = hdr_sz;
			assert(encoded); assert(enc_len); assert(rtp_hdr); assert(rtp_hdr_len);
/*			auto sent = m_socket.send_to(std::array<asio::const_buffer, 2>{asio::buffer(encoded, enc_len), asio::buffer(rtp_hdr, rtp_hdr_len)}
				, m_remote_media);
			assert(sent);*/

			// Reset media element
			reset();
		}
		else
		{
			break;
		}
	}
}


} //namespace cliph::sound