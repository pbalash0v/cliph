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
#include "data_types.hpp"


namespace cliph::net
{

std::vector<asio::ip::address> get_interfaces();

struct config
{
	inline static constexpr const auto k_rtp_advance_interval = rtpp::stream::duration_type{kPeriodSizeInMilliseconds};
};

class socket final
{
public:
	socket(asio::ip::address, data::media_buf&);
	~socket();

public:
	void run();
	void stop();
	std::uint16_t port() const noexcept;
	void write(const asio::ip::udp::endpoint&, const void*, std::size_t);
	void set_net_sink(std::string_view ip, std::uint16_t port);

private:
	asio::io_context m_io;
	data::media_buf& m_buf;
	asio::ip::udp::socket m_socket;//{m_io};//, asio::ip::udp::endpoint {asio::ip::udp::v4(), 0u}};
	//
	std::thread m_thr;
	//
	asio::ip::address m_local_media;
	//
	asio::ip::udp::endpoint m_remote_media;

private:
	void read_loop();
};

} //namespace cliph::net

#endif // net_hpp