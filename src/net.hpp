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
	asio::ip::address m_local_media{};
};

class engine final
{
public:
	engine(const config&, data::media_buf&, data::media_queue&, data::media_buf&);
	~engine();

public:
	void run();
	void stop();
	std::uint16_t port() const noexcept;
	void set_remote(std::string_view ip, std::uint16_t port);

private:
	//! from pipeline to net
	data::media_buf& m_egress_buf;
	//! from pipeline to net
	data::media_queue& m_ingress_audio_q;
	//! from this module to rtp module
	data::media_buf& m_ingress_rtp_buf;

	asio::io_context m_io;
	asio::ip::udp::socket m_socket;
	//
	std::thread m_ingress_thr;
	std::thread m_egress_thr;
	//
	asio::ip::address m_local_media;
	//
	asio::ip::udp::endpoint m_remote_media;

private:
	void ingress_loop();
	void egress_loop();

};

} //namespace cliph::net

#endif // net_hpp