#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
extern "C"
{
#include <netinet/in.h>
}
#include "rtp.hpp"


using octet_type = std::uint8_t;


namespace cliph::rtp
{
rtp::rtp(void* start, std::size_t len)
	: m_start{start}
	, m_len{len}
{
	if ((start == nullptr) or (len == 0))
	{
		throw std::runtime_error{"invalid packet, start is nullptr or size is 0"};
	}
}


[[nodiscard]] const void* rtp::data() const noexcept
{
	constexpr const auto mandatory_fields_len = 3u*sizeof(std::uint32_t);

	const auto* ext_data = has_extensions()
		? static_cast<octet_type*>(m_start) + mandatory_fields_len + csrc_count()*sizeof(std::uint32_t) : nullptr;
	const auto ehl = [&]()
	{
		if (ext_data)
		{
			return ::ntohs(*reinterpret_cast<const std::uint16_t*>(ext_data + sizeof(std::uint16_t)));
		}
		else
		{
			return std::uint16_t{};
		}
	}();

	return static_cast<octet_type*>(m_start) + mandatory_fields_len + (csrc_count()*sizeof(std::uint32_t))
		+ has_extensions()*(sizeof(std::uint32_t) + ehl*sizeof(std::uint32_t));
}
void* rtp::data() noexcept
{
	return const_cast<void*>(const_cast<const rtp*>(this)->data());
}

[[nodiscard]] std::uint16_t rtp::size() const noexcept
{
	return m_len - (static_cast<const octet_type*>(data()) - static_cast<octet_type*>(m_start));
}

//
[[nodiscard]] std::uint8_t rtp::csrc_count() const noexcept
{
	constexpr const auto k_cc_mask = 0b0000'1111u;
	return (*static_cast<octet_type*>(m_start)) & k_cc_mask;
}
void rtp::csrc_count(std::uint8_t count)
{
	constexpr const auto k_cc_mask_clear = 0b1111'0000u;

	if (count > 15u) { throw std::runtime_error{"Allowed CSRC range is [0,15], but given value is: " + std::to_string(count)}; }
	auto& val = *static_cast<octet_type*>(m_start);
	val &= k_cc_mask_clear;
	val |= count;
}
//
void rtp::ver(std::uint8_t v) noexcept
{
	constexpr const auto k_version_2_set = 0b1000'0000u;
	constexpr const auto k_version_2_clear = 0b1011'1111u;

	auto& octet = *(static_cast<octet_type*>(m_start));
	octet |= k_version_2_set;
	octet &= k_version_2_clear;

	assert(*this);
}
//
bool rtp::is_padded() const noexcept
{
	constexpr const auto k_padding_mask = 0b0010'0000u;
	return (*static_cast<octet_type*>(m_start)) & k_padding_mask;
}
void rtp::pad(bool p) noexcept
{
	assert(*this);
	constexpr const auto k_padding_mask_set = 0b0010'0000u;
	constexpr const auto k_padding_mask_clear = 0b1101'1111u;
	auto& octet = *static_cast<octet_type*>(m_start);
	p ? (octet |= k_padding_mask_set) : (octet &= k_padding_mask_clear);
	assert(*this);	
}
//
bool rtp::has_extensions() const noexcept
{
	constexpr const auto k_x_mask =	0b0001'0000u;
	const auto val = *static_cast<octet_type*>(m_start);
	return val & k_x_mask;
}
void rtp::extensions(bool x) noexcept
{
	assert(*this);
	constexpr const auto k_ext_mask_set = 0b0001'0000u;
	constexpr const auto k_ext_mask_clear = 0b1110'1111u;

	auto& octet = *static_cast<octet_type*>(m_start);
	x ? (octet |= k_ext_mask_set) : (octet &= k_ext_mask_clear);
	assert(*this);
}
//
bool rtp::has_mark() const noexcept
{
	constexpr const auto k_mark_mask = 0b1000'0000u;
	return (*(static_cast<octet_type*>(m_start) + 1u)) & k_mark_mask;
}
void rtp::mark(bool m) noexcept
{
	assert(*this);
	constexpr const auto k_mark_mask_set = 0b1000'0000u;
	constexpr const auto k_mark_mask_clear = 0b0111'1111u;
	auto& octet = *(static_cast<octet_type*>(m_start) + 1u);
	m ? (octet |= k_mark_mask_set) : (octet &= k_mark_mask_clear);
	assert(*this);
}
//
std::uint8_t rtp::pt() const noexcept
{
	constexpr const auto k_pt_mask = 0b0111'1111u;
	return (*(static_cast<octet_type*>(m_start) + 1u)) & k_pt_mask;
}
void rtp::pt(std::uint8_t pt)
{
	if (pt > 127u) { throw std::runtime_error{"Allowed PT range is [0,127], but given value is: " + std::to_string(pt)}; }
	auto& val = *(static_cast<octet_type*>(m_start) + 1u);
	val = pt;
}
//


/*
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
*/
std::uint16_t rtp::seq_num() const noexcept
{
	const auto val = *(reinterpret_cast<std::uint16_t*>(m_start) + 1u);
	return ::ntohs(val);
}
void rtp::seq_num(std::uint16_t sn) noexcept
{
	auto& val = *(reinterpret_cast<std::uint16_t*>(m_start) + 1u);
	val = ::htons(sn);
}
//
std::uint32_t rtp::ts() const noexcept
{
	const auto val = *(reinterpret_cast<std::uint32_t*>(m_start) + 1u);
	return ::ntohl(val);
}
void rtp::ts(std::uint32_t ts) noexcept
{
	auto& ptr = *(reinterpret_cast<std::uint32_t*>(m_start) + 1u);
	ptr = ::htonl(ts);
}
//
std::uint32_t rtp::ssrc() const noexcept
{
	const auto ptr = *(reinterpret_cast<std::uint32_t*>(m_start) + 2u);
	return ::ntohl(ptr);
}
void rtp::ssrc(std::uint32_t ssrc_val) noexcept
{
	auto& val = *(reinterpret_cast<std::uint32_t*>(m_start) + 2u);
	val = ::htonl(ssrc_val);
}

//
rtp::operator const std::uint8_t*() const noexcept
{
	return static_cast<const std::uint8_t*>(data());
}

rtp::operator bool() const noexcept
{
	constexpr const auto k_version_mask = 0b1100'0000u;
	constexpr const auto k_version_2 = 0b1000'0000u;
	auto octet_val = *(static_cast<octet_type*>(m_start));
	octet_val &= k_version_mask;
	return octet_val == k_version_2;
}

std::ostream& rtp::dump(std::ostream& ostr) const
{
	ostr << std::boolalpha
		<< "RTP [pad:" << is_padded()
		<< ", mark:" << has_mark()
		<< ", x:" << has_extensions()
		<< ", pt:" << +pt()
		<< ", seq:" << +seq_num()
		<< ", ts:" << +ts()
		<< ", ssrc:" << +ssrc()
		<< ", len:" << size()
		<< "]";
	
	return ostr;	
}



} //namespace mspg