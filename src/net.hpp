#ifndef net_hpp
#define net_hpp

#include <cstdint>
//
#include "asio/io_context.hpp"
#include "asio/ip/udp.hpp"
#include "asio.hpp"


namespace cliph::net
{
class socket final
{
public:
	std::uint16_t port() const noexcept;
	void write(const asio::ip::udp::endpoint&, const void*, std::size_t);

private:
	asio::io_context m_io;
	asio::ip::udp::socket m_socket{m_io, asio::ip::udp::endpoint {asio::ip::udp::v4(), 0u}};
};

} //namespace cliph::net

#endif // net_hpp