#ifndef net_hpp
#define net_hpp

#include <atomic>
#include <cstdint>
//
#include "asio/io_context.hpp"
#include "asio/ip/udp.hpp"
#include "asio.hpp"
//
#include "utils.hpp"


namespace cliph::net
{
class socket final
{
public:
	socket(utils::thread_safe_array&);
	~socket();

public:
	void run();
	void stop();
	std::uint16_t port() const noexcept;
	void write(const asio::ip::udp::endpoint&, const void*, std::size_t);

private:
	utils::thread_safe_array& m_buf;
	asio::io_context m_io;
	asio::ip::udp::socket m_socket{m_io, asio::ip::udp::endpoint {asio::ip::udp::v4(), 0u}};
	std::thread m_read_thread;
	std::atomic_bool m_should_run{true};

private:
	void read_loop();
};

} //namespace cliph::net

#endif // net_hpp