#include "rtp.hpp"
#include "rtpp/rtp.hpp"

namespace cliph::rtp
{
engine::engine(data::media_buf& in, data::media_buf& out)
	: m_in_buf{in}
	, m_out_buf{out}
{
	m_strm.payloads().emplace(96,rtpp::payload_type{48000u});
}

engine::~engine()
{
	if (m_egress_thr.joinable()) { m_egress_thr.join(); }
	if (m_ingress_thr.joinable()) { m_ingress_thr.join(); }
}

void engine::run()
{
	m_egress_thr = std::thread{[this]()
	{
		std::cerr << "rtp egress_loop started\n";
		egress_loop();
		std::cerr << "rtp egress_loop finished\n";
	}};

	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_RTP"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void engine::egress_loop()
{
	while (true)
	{
		auto slot = data::slot_type{};
		m_in_buf.get(slot);
		//
		if (slot->raw_audio_sz == 0) { break; }

		//
		const auto encoded_audio_len = slot->rtp_data_sz;
		const auto raw_audio_len = slot->raw_audio_len;

		m_strm.advance_ts(96, raw_audio_len);
		if (encoded_audio_len == 0) { continue; } // dtx

		m_strm.advance_seq_num();
		const auto hdr_sz = static_cast<unsigned char*>(m_strm.fill(slot->rtp_hdr.data(), slot->rtp_hdr.size())) - slot->rtp_hdr.data();
		std::cerr << rtpp::rtp{slot->rtp_hdr.data(), static_cast<size_t>(hdr_sz)} << ", hdr_sz: " <<  hdr_sz << '\n';
	}
}

} //namespace cliph::rtp