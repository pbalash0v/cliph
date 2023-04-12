#include "data_types.hpp"
#include <array>
#include <math.h>
extern "C"
{
#include <sys/types.h> // int getifaddrs(struct ifaddrs **ifap); void freeifaddrs(struct ifaddrs *ifa);
#include <ifaddrs.h>
#include <netdb.h>
#include <linux/if_link.h>
}
#include <iostream>
#include <mutex>
#include <memory>
#include <thread>
//
#include "asio/buffer.hpp"
//
#include "net.hpp"


namespace cliph::net
{

engine::engine(const config& cfg
	, data::media_buf& from_rtp
	, data::media_queue& q
	, data::media_buf& to_rtp)
	: m_egress_buf{from_rtp}
	, m_ingress_audio_q{q}
	, m_ingress_rtp_buf{to_rtp}
	, m_socket{asio::ip::udp::socket{m_io, asio::ip::udp::endpoint{cfg.m_local_media, 0u}}}
{
	m_io.get_executor().on_work_started();
}

engine::~engine()
{
	stop();
	if (m_ingress_thr.joinable()) { m_ingress_thr.join(); }
	if (m_egress_thr.joinable()) { m_egress_thr.join(); }
}

std::uint16_t engine::port() const noexcept
{
	return m_socket.local_endpoint().port();
}

void engine::set_remote(std::string_view ip, std::uint16_t port)
{
	m_io.post([ip=std::string{ip}, port, this]
	{
		m_remote_media = asio::ip::udp::endpoint{asio::ip::make_address_v4(ip), port};
	});
}

void engine::run()
{
	m_ingress_thr = std::thread{[this]()
	{
		std::cerr << "net ingress_loop started\n";
		ingress_loop();
		std::cerr << "net ingress_loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_ingress_thr.native_handle(), "INGRS_NET_IO"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
	//
	m_egress_thr = std::thread{[this]()
	{
		std::cerr << "net egress_loop started\n";
		egress_loop();
		std::cerr << "net egress_loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_NET_IO"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void engine::stop()
{
	m_io.get_executor().on_work_finished();
	m_socket.close();
	m_io.stop();
}

void engine::egress_loop()
{
	while (true)
	{
		auto slot = data::slot_type{};
		m_egress_buf.get(slot);
		//
		if (slot->raw_audio_sz == 0) { break; }

		//
		const auto* encoded = slot->rtp_data.data();
		const auto enc_len = slot->rtp_data_sz;
		const auto* rtp_hdr = slot->rtp_hdr.data();
		const auto rtp_hdr_len = slot->rtp_hdr_sz;
		assert(encoded); assert(enc_len); assert(rtp_hdr); assert(rtp_hdr_len);
		auto sent = m_socket.send_to(std::array<asio::const_buffer, 2>{asio::buffer(encoded, enc_len), asio::buffer(rtp_hdr, rtp_hdr_len)}
			, m_remote_media);
		assert(sent);
	}
}

void engine::ingress_loop()
{
	while (true)
	{
		auto slot = m_ingress_audio_q.get();
		if (!slot) continue;
		slot->reset();
		auto from = decltype(m_socket)::endpoint_type{};
		auto recvd = m_socket.receive_from(asio::buffer(slot->rtp_data), from);
		slot->rtp_data_sz = recvd;

		m_ingress_rtp_buf.put(std::move(slot));
	}
}

} // namespace cliph::net


namespace std
{
template<>
class default_delete<struct ifaddrs>
{
public:
	void operator()(ifaddrs* ifa) const noexcept
	{
		::freeifaddrs(ifa);
	}
};
} // namespace std

namespace cliph::net
{

std::vector<asio::ip::address> get_interfaces()
{
	using ifaddrs_type = struct ifaddrs;
	using ifaddrs_ptr_type = ifaddrs_type*;
	using ifaddrs_uptr_type = std::unique_ptr<ifaddrs_type>;

	auto ret = std::vector<asio::ip::address>{};

	auto* if_addrs_ptr = ifaddrs_ptr_type{};
	if (::getifaddrs(&if_addrs_ptr) == -1)
	{
		throw std::runtime_error{std::string{"getifaddrs() failed: "} + ::strerror(errno)};
	}
	auto if_addrs = ifaddrs_uptr_type{if_addrs_ptr};

 	auto family = int{};
	char host[NI_MAXHOST] = {};
	for (auto* ifa = if_addrs_ptr; ifa != NULL;	ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
		{
			continue;
		}

		family = ifa->ifa_addr->sa_family;
#if 0
		/* Display interface name and family (including symbolic
		form of the latter for the common families). */
		printf("%-8s %s (%d)\n",
		ifa->ifa_name,
		(family == AF_PACKET) ? "AF_PACKET" :
		(family == AF_INET) ? "AF_INET" :
		(family == AF_INET6) ? "AF_INET6" : "???",
		family);
#endif
		/* For an AF_INET* interface address, display the address. */
		if (family == AF_INET)
		{
			if (auto s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in)
				, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST
				); s != 0)
			{
				throw std::runtime_error{std::string{"getnameinfo() failed: "} + ::gai_strerror(s)};
			}
			else
			{
				if (auto addr = asio::ip::make_address(host)
					; addr.is_loopback()or addr.is_unspecified())
				{
					continue;
				}
				else
				{
					ret.push_back(std::move(addr));
				}
			}
			//printf("\t\taddress: <%s>\n", host);
		}
/*		else if (family == AF_PACKET && ifa->ifa_data != NULL)
		{
			auto* stats = static_cast<struct rtnl_link_stats*>(ifa->ifa_data);

			printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
			"\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
			stats->tx_packets, stats->rx_packets,
			stats->tx_bytes, stats->rx_bytes);
		}*/
	}

	return ret;
}

} // namespace cliph::net
