#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
//
#include "rtp_stream.hpp"
#include "rtp.hpp"

namespace
{
constexpr const auto k_pcmu_pt = 0u;
constexpr const auto k_pcmu_clock = 8000u;
constexpr const auto k_pcma_pt = 0u;
constexpr const auto k_pcma_clock = 8000u;

const auto k_default_profiles = cliph::rtp::stream::payload_map_type{
	{k_pcmu_pt, {k_pcmu_clock}},
	{k_pcma_pt, {k_pcma_clock}},
};
} // namespace

namespace
{
std::random_device rd{};
std::mt19937 mt{rd()};
std::uniform_int_distribution distrib_uint32{std::numeric_limits<std::uint32_t>::min(), std::numeric_limits<std::uint32_t>::max()};
std::uniform_int_distribution distrib_uint16{std::numeric_limits<std::uint16_t>::min(), std::numeric_limits<std::uint16_t>::max()};
} // namespace


namespace cliph::rtp
{
stream::stream()
	: m_seq_num{distrib_uint16(mt)}
	, m_ssrc{distrib_uint32(mt)}
	, m_ts{(distrib_uint32(mt)/10)*10}
	, m_payloads{k_default_profiles}
{
}

void stream::advance_ts(pt_type pt, duration_type duration)
{
	m_pt = pt;
	// e.g. this would be 8 for PCM 8000 samples/second,
	// 48 for OPUS 48000 samples/second etc
	const auto samples_per_ms = m_payloads.at(pt).m_clock / std::chrono::milliseconds{1000}.count();
	m_ts += samples_per_ms * duration.count();
}

void stream::advance_seq_num() noexcept
{
	++m_seq_num;
}

void* stream::fill(void* start, std::size_t len, bool mark)
{
	auto rtp_pkt = cliph::rtp::rtp{start, len};
	rtp_pkt.ver();
	rtp_pkt.mark(mark);
	rtp_pkt.csrc_count(m_csrc_count);
	rtp_pkt.seq_num(m_seq_num);
	rtp_pkt.ts(m_ts);
	rtp_pkt.pt(m_pt);
	rtp_pkt.extensions(false);
	rtp_pkt.ssrc(m_ssrc);

	return rtp_pkt.data();
}


std::ostream& stream::dump(std::ostream& ostr) const
{
	ostr << "RTP stream [SSRC:" << m_ssrc
		<< ", seq:" << m_seq_num
		<< ", ts:" << m_ts
		<< "]";
	
	return ostr;	
}
} //namespace cliph::rtp
