#include "net.hpp"

namespace cliph::net
{
std::uint16_t socket::port() const noexcept
{
	return m_socket.local_endpoint().port();
}

void socket::write(const asio::ip::udp::endpoint& destination, const void* buf, std::size_t len)
{
	m_socket.send_to(asio::buffer(buf, len), destination);
}


} // namespace cliph::net
