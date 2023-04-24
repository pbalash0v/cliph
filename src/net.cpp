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
