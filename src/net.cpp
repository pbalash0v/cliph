#include <iostream>
#include <mutex>
#include <thread>
//
#include "asio/buffer.hpp"
//
#include "net.hpp"


namespace cliph::net
{
socket::socket(utils::thread_safe_array& buf)
	: m_buf{buf}
{
}

socket::~socket()
{
	stop();
	if (m_read_thread.joinable()) m_read_thread.join();
}

std::uint16_t socket::port() const noexcept
{
	return m_socket.local_endpoint().port();
}

void socket::write(const asio::ip::udp::endpoint& destination, const void* buf, std::size_t len)
{
	m_socket.send_to(asio::buffer(buf, len), destination);
}

void socket::run()
{
	m_read_thread = std::thread{[&]()
	{
		read_loop();
	}};
}

void socket::stop()
{
	m_should_run = false;
	m_socket.close();
	m_io.stop();
}

void socket::read_loop()
{
	while (m_should_run)
	{
		auto u_lock = std::unique_lock{m_buf.m_mtx};
		m_buf.m_cond.wait(u_lock, [&]()
		{
			return m_buf.m_is_empty;
		});
		u_lock.unlock();

		auto recvd = m_socket.receive(asio::buffer(m_buf.m_buffer));

		{
			auto lock_g = std::lock_guard{m_buf.m_mtx};
			m_buf.m_is_empty = false;
			m_buf.m_size = recvd;
		}
	}
}

} // namespace cliph::net
