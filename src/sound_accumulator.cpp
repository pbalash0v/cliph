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
#include "data_types.hpp"
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
	m_opus_enc_ctx = get_opus_enc(m_snd_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u, /*dtx */true);
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
		std::cerr << "accumulator egress_loop started\n";
		egress_loop();
		std::cerr << "accumulator egress_loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
#if 0
	m_ingress_thr = std::thread{[this]()
	{
		std::cerr << "accumulator ingress_loop started\n";
		ingress_loop();
		std::cerr << "accumulator ingress_loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_ingress_thr.native_handle(), "IGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
#endif		
}

void accumulator::ingress_loop()
{
#if 0	
	while (true)
	{
		
		auto slot = m_ingress_audio_q.get();
		if (!slot) continue;
		slot->reset();
		auto from = decltype(m_socket)::endpoint_type{};
		auto recvd = m_socket.receive_from(asio::buffer(slot->rtp_data), from);
		slot->rtp_data_sz = recvd;

		m_ingress_rtp_buf.put(std::move(slot));
	}
#endif
}

void accumulator::egress_loop()
{
	// To encode a frame opus_encode() must be called with exactly one frame (2.5, 5, 10, 20, 40 or 60 ms) of audio data
	const auto k_lengths = std::array<milliseconds, 4u>{10ms, 20ms, 40ms, 60ms};
	//
	using slot_type = typename std::remove_reference_t<decltype(m_out_q)>::slot_type;

	//
	bool is_dtx = false;
	//
	const auto reset = [&](auto& media)
	{
		media.reset();
		m_egress_snd_offset = 0;
		m_next_len = k_lengths[std::rand() % k_lengths.size()];
	};

	while (true)
	{
		auto slot = slot_type{};

		m_out_strm.get(slot);
		auto& sound = slot->m_sound;
		auto& rtp = slot->m_rtp;
		//
		if (sound.sample_count == 0u) { break; }

		std::memcpy(m_egress_snd_accum.data() + m_egress_snd_offset, sound.data.data(), sound.bytes_sz());
		m_egress_snd_offset += sound.bytes_sz();
		//
		const auto samples_per_ms = sound.sample_rate/1000u;
		const auto curr_accum = milliseconds{(m_egress_snd_offset/sizeof(cliph::data::media_elt::sound::sample_type))/samples_per_ms};
		//
		//std::printf("%lu bytes, %d samples, %d ms\n" \
			, sound.bytes_sz(), sound.sample_count, sound.sample_count/samples_per_ms);
		assert(milliseconds{sound.sample_count/samples_per_ms} == 10ms);
		//
		if (curr_accum < m_next_len) { continue; }

		// encode
		if (const auto encoded = ::opus_encode(m_opus_enc_ctx
			, m_egress_snd_accum.data(), m_egress_snd_offset
			, rtp.data.data(), rtp.data.size())
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
				rtp.data_sz = encoded;
				std::printf("%ld ms, encoded: %d\n", curr_accum.count(), encoded);
			}
		}

		// packetize
		m_strm.advance_ts(96u, curr_accum);
		if (is_dtx)
		{
			reset(*slot);
			continue;
		}
		m_strm.advance_seq_num();

		const auto hdr_sz = static_cast<unsigned char*>(m_strm.fill(rtp.hdr.data(), rtp.hdr.size())) - rtp.hdr.data();
		//std::cerr << rtpp::rtp{media.rtp_hdr.data(), static_cast<size_t>(hdr_sz)} << ", hdr_sz: " <<  hdr_sz << '\n';

		// send
		const auto* rtp_hdr = rtp.hdr.data();
		const auto rtp_hdr_len = hdr_sz;			
		const auto* encoded = rtp.data.data();
		const auto enc_len = rtp.data_sz;
		assert(rtp_hdr); assert(rtp_hdr_len); assert(encoded); assert(enc_len);
		using rtp_type = std::array<asio::const_buffer, 2>;
		auto sent = m_socket.send_to(rtp_type{asio::buffer(rtp_hdr, rtp_hdr_len), asio::buffer(encoded, enc_len)}
			, m_remote_media);
		assert(sent);

		// Reset media element
		reset(*slot);
	}
}


} //namespace cliph::sound