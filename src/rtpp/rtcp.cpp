#include <cstdint>
#include <stdexcept>
extern "C"
{
#include <netinet/in.h>
}
#include "rtcp.hpp"

using octet_type = std::uint8_t;

namespace rtpp
{

rtcp::rtcp(void* start, std::size_t len)
	: m_start{start}
	, m_len{len}
{
	if ((start == nullptr) or (len == 0))
	{
		throw std::runtime_error{"invalid packet, start is nullptr or size is 0"};
	}

	if (ver() != 2u)
	{
		throw std::runtime_error{"Unsupported version: " + std::to_string(ver())};
	}
}

rtcp::operator bool() const noexcept
{
	const auto* rtcp_type = static_cast<octet_type*>(m_start) + 1u;
	// https://datatracker.ietf.org/doc/html/rfc5761#section-4
	return (*rtcp_type >= 192u) and (*rtcp_type <= 223u);
}


std::uint8_t rtcp::ver() const noexcept
{
	constexpr const auto k_version_mask = 0b1100'0000u;
	return ((*static_cast<octet_type*>(m_start)) & k_version_mask) >> 6u;
}

bool rtcp::is_padded() const noexcept
{
	constexpr const auto k_padding_mask = 0b0010'0000u;
	return (*static_cast<octet_type*>(m_start)) & k_padding_mask;
}



} //namespace mspg