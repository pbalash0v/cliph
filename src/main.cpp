#include <cstdlib>
#include <iterator>
#include <random>
#include <stdio.h>
#include <stdlib.h>
//
#include <ios>
#include <iostream>
#include <atomic>
//
#include "argh.h"
//
#include "audio_engine.hpp"
#include "net.hpp"
#include "sip_agent.hpp"
#include "sdp.hpp"

namespace
{

struct config final
{
	cliph::sip::call_config sip;
	cliph::config media;
};

std::optional<config> get(char** argv)
{
	auto ret = config{};

	auto cmdl = argh::parser{argv};

	if (cmdl[{"-h", "--help" }])
	{
		return std::nullopt;
	}
	//
	if (auto callee_uri = cmdl({"-d", "dest"}).str(); not callee_uri.empty())
	{
		ret.sip.to = std::move(callee_uri);
	}
	else
	{
		return std::nullopt;
	}
	//
	if (auto caller_uri = cmdl({"-u", "uri"}).str(); not caller_uri.empty())
	{
		ret.sip.from = std::move(caller_uri);
	}
	if (auto auth_username = cmdl({"-a", "auth"}).str(); not auth_username.empty())
	{
		ret.sip.auth = std::move(auth_username);
	}
	if (auto password = cmdl({"-p", "pswd"}).str(); not password.empty())
	{
		ret.sip.pswd = std::move(password);
	}
	if (auto out_prx = cmdl({"-o", "prx"}).str(); not out_prx.empty())
	{
		ret.sip.outbound_prx = std::move(out_prx);
	}

    return ret;
}

void print_usage(std::string_view binary)
{
	std::cout << "Usage: cliph [-hudap]" << '\n';
	std::cout << "\tExample: " << binary
		<< " -u=sip:caller@domain.com -d=sip:callee@domain.com -o=sip:proxy.domain.com -a=auth_user_name -p=auth_password"
		<< std::endl;
};

auto get_local_media_ip()
{
	if (auto local_ifaces = cliph::net::get_interfaces(); local_ifaces.size() == 1)
	{
		std::cerr << local_ifaces.front() << " is used for media stream" << '\n';
		return local_ifaces.front();
	}
	else
	{
		auto iface_idx = 0u;
		while (true)
		{
			std::cout << "Input local network interface for media stream: [0-" << local_ifaces.size() -1 << "]" << std::endl;
			for (auto it = std::cbegin(local_ifaces), fin = std::cend(local_ifaces); it != fin; ++it)
			{
				std::cout << std::distance(std::cbegin(local_ifaces), it) << ": " << *it << std::endl;
			}
			//
			std::cin >> iface_idx;
		    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			if (!std::cin.good())
			{
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				continue;
			}

			if (iface_idx < local_ifaces.size())
			{
				std::cerr << local_ifaces[iface_idx] << " selected" << '\n';
				return local_ifaces[iface_idx];
			}
			else
			{
				std::cerr << "wrong selection" << '\n';
			}
		}
	}
}

}// namespace

int main(int /*argc*/, char** argv)
{
	if (auto cfg = get(argv))
	{
		cfg->media.net_iface = get_local_media_ip();

		cliph::engine::get().init(cfg->media);
		cliph::sip::agent::get().run(cfg->sip);
		//
		std::printf("Press Enter to stop...\n");
		getchar();
		std::printf("Terminating...\n");
		//
		cliph::sip::agent::get().stop();
	}
	else
	{
		print_usage(argv[0]);
	}

	return EXIT_SUCCESS;
}