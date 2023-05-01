#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ratio>
#include <stdexcept>
#include <thread>
//
#include <rtp.hpp>
#include <unistd.h>
#include "data_types.hpp"
#include "opus.h"
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
	, data::media_queue& out_q
	, data::media_stream& out_strm
	, data::media_queue& in_q
	, data::media_stream& in_strm)
	: m_controller{controller}
	, m_snd_cfg{cfg.m_snd}
	, m_out_q{out_q}
	, m_out_strm{out_strm}
	, m_in_q{in_q}
	, m_in_strm{in_strm}
	, m_socket{asio::ip::udp::socket{m_io, asio::ip::udp::endpoint{cfg.local_media_ip, 0u}}}
{
	m_opus_enc_ctx = get_opus_enc(m_snd_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u, /*dtx */false);
	m_opus_dec_ctx = get_opus_dec(m_snd_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u);

	m_strm.payloads().emplace(96, rtpp::payload_type{48000u});
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
		std::cerr << "egress accumulator loop started\n";
		egress_loop();
		std::cerr << "egress accumulator loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}

	m_ingress_thr = std::thread{[this]()
	{
		std::cerr << "ingress accumulator loop started\n";
		ingress_loop();
		std::cerr << "ingress accumulator loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_ingress_thr.native_handle(), "IGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void accumulator::ingress_loop(bool debug)
{
	while (true)
	{
		auto net_in_slot = m_in_q.get();
		if (!net_in_slot)
		{
			std::cerr << "Ingress audio buffer overrun\n";
			continue;
		}

		//
		net_in_slot->reset();
		auto& rtp = net_in_slot->m_rtp;
		//
		// Net
		//
		auto from = decltype(m_socket)::endpoint_type{};
		auto recvd = m_socket.receive_from(asio::buffer(rtp.data), from);
		rtp.data_sz = recvd;
		//
		// RTP
		//
		const auto rtp_pkt = rtpp::rtp{rtp.data.data(), rtp.data_sz};
		//std::cerr << rtp_pkt << '\n';
		if (not rtp_pkt)
		{
			net_in_slot->reset();
			continue;
		}
		// we only offer OPUS and nothing else, so only expect OPUS payloads
		// TODO: incoming pt check
		if (rtp_pkt.pt() != 96u) // FIXME !!!!!!!!!!!!!!!
		{
			std::cerr << "Got unexpected pt: " <<  rtp_pkt.pt() << '\n';
			net_in_slot->reset();
			continue;
		}
		//
		// Media
		//
		auto samples_decoded = ::opus_decode(m_opus_dec_ctx
			, rtp_pkt, rtp_pkt.size()
			, m_ingress_snd_accum.data() + m_ingress_snd_offset, m_ingress_snd_accum.size() - m_ingress_snd_offset
			, /*decode_fec*/ 0);
		if (samples_decoded < 0)
		{
			std::fprintf(stderr, "%s\n", get_opus_err_str(samples_decoded).data());
			continue;
		}
		debug and std::fprintf(stderr, "[%s] decoded: %d\n", __func__, samples_decoded);
		//
		const auto k_opus_rtp_clocking = 48000u;
		const auto samples_per_ms = k_opus_rtp_clocking/1000u;
		const auto cur_frame_sz_ms = std::chrono::milliseconds{samples_decoded/samples_per_ms};
		if ((cur_frame_sz_ms.count() % 10 != 0) or (cur_frame_sz_ms != 20ms))
		{
			throw std::runtime_error{"Unhandled payload size"};
		}

		const auto needed_slot_count = cur_frame_sz_ms.count() / 10u;
		for (auto slot_num = 0u, snd_offset = 0u; slot_num != needed_slot_count; ++slot_num)
		{
			if (auto slot = m_in_q.get())
			{
				slot->reset();
				slot->m_sound.sz = 10ms;
				slot->m_sound.sample_count = samples_decoded / needed_slot_count;
				slot->m_sound.sample_rate = 48000u;
				std::memcpy(slot->m_sound.data.data()
					, m_ingress_snd_accum.data() + snd_offset, 2*slot->m_sound.sample_count);
				snd_offset += slot->m_sound.sample_count;
				debug && std::fprintf(stderr, "[%s]: %lu samples, %lu bytes\n"
					, __func__, slot->m_sound.sample_count, 2*slot->m_sound.sample_count);
				m_in_strm.put(std::move(slot));
			}
			else
			{
				debug && std::cerr << "Ingress audio buffer overrun\n";
				continue;
			}
		}

		net_in_slot->reset();
	}
}

void accumulator::egress_loop(bool debug)
{
	// To encode a frame opus_encode() must be called with exactly one frame (2.5, 5, 10, 20, 40 or 60 ms) of audio data
	// But OPUS_APPLICATION_VOIP type encoding only supports (10, 20, 40 or 60 ms) of audio data
	const auto k_lengths = std::array<milliseconds, 4u>{10ms, 20ms, 40ms, 60ms};
	//
	using slot_type = typename std::remove_reference_t<decltype(m_out_q)>::slot_type;

	//
	const auto reset = [&](auto& media)
	{
		media.reset();
		m_egress_snd_sz = 0ms;
		m_egress_sample_count = 0;
		m_egress_bytes_count = 0;
		m_next_len = k_lengths[std::rand() % k_lengths.size()];
	};

	while (true)
	{
		auto slot = slot_type{};

		m_out_strm.get(slot);
		//
		auto& sound = slot->m_sound;
		auto& rtp = slot->m_rtp;
		if (sound.sample_count == 0u or sound.sz == 0ms) { break; }
		//
		// Accumulate
		//
		assert(sound.sz == 10ms);
		assert(sound.sample_rate == 48000);
		assert(sound.sample_count == 480);
		assert(sound.bytes_count == 480*2);
		//
		std::memcpy((char*)m_egress_snd_accum.data() + m_egress_bytes_count, sound.data.data(), sound.bytes_count);
		m_egress_sample_count += sound.sample_count;
		m_egress_snd_sz += sound.sz;
		m_egress_bytes_count += sound.bytes_count;
		//
		if (m_egress_snd_sz != m_next_len) { slot->reset(); continue; }

		//
		// Encode
		//
		debug and std::fprintf(stderr, "[%s] accumulated %ld ms, %lu samples, %lu bytes of audio\n"
					, __func__, m_egress_snd_sz.count(), m_egress_sample_count, m_egress_bytes_count);
		const auto encoded = ::opus_encode(m_opus_enc_ctx
			, static_cast<const opus_int16*>(m_egress_snd_accum.data()), m_egress_sample_count
			, rtp.data.data(), rtp.data.size());
		if (encoded < 0)
		{
			std::cerr << get_opus_err_str(encoded) << '\n';
			reset(*slot);
			continue;
		}
		//
		// Packetize
		//
		m_strm.advance_ts(96u, m_egress_snd_sz);
		if (encoded < 3) // DTX
		{
			debug and std::fprintf(stderr, "DTX\n");
			reset(*slot);
			continue;
		}
		//
		debug and std::fprintf(stderr, "[%s] total of %ld ms of audio encoded to %d bytes\n"
			, __func__, m_egress_snd_sz.count(), encoded);

		m_strm.advance_seq_num();

		//
		// send
		//
		const auto* hdr = rtp.hdr.data();
		const auto hdr_sz = static_cast<unsigned char*>(m_strm.fill(rtp.hdr.data(), rtp.hdr.size())) - rtp.hdr.data();
		const auto* data = rtp.data.data();
		debug and std::cerr << rtpp::rtp{rtp.hdr.data(), static_cast<size_t>(hdr_sz)} << ", hdr_sz: " <<  hdr_sz << '\n';
		using rtp_type = std::array<asio::const_buffer, 2>;
		auto sent = m_socket.send_to(rtp_type{asio::buffer(hdr, hdr_sz), asio::buffer(data, encoded)}
			, m_remote_media);
		assert(sent);

		// Reset media element
		reset(*slot);
	}
}


} //namespace cliph::sound