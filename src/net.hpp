#ifndef net_hpp
#define net_hpp

#include <atomic>
#include <cstdint>
//
#include "asio/io_context.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/udp.hpp"
#include "asio.hpp"
//
#include "utils.hpp"


namespace cliph::net
{

std::vector<asio::ip::address> get_interfaces();

class socket final
{
public:
	socket(asio::ip::address, utils::thread_safe_array&);
	~socket();

public:
	void run();
	void stop();
	std::uint16_t port() const noexcept;
	void write(const asio::ip::udp::endpoint&, const void*, std::size_t);

private:
	asio::io_context m_io;
	utils::thread_safe_array& m_buf;
	asio::ip::udp::socket m_socket;//{m_io};//, asio::ip::udp::endpoint {asio::ip::udp::v4(), 0u}};
	//
	std::thread m_read_thread;

private:
	void read_loop();
};

} //namespace cliph::net

#endif // net_hpp