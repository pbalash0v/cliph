#include <iostream>
#include <optional>
//
#include "argh.h"
//
#include "controller.hpp"

namespace
{

std::optional<cliph::config> get(char** argv)
{
	auto ret = cliph::config{};

	auto cmdl = argh::parser{argv};

	if (cmdl[{"-h", "--help" }])
	{
		return std::nullopt;
	}
#if 0	
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
	if (cmdl[{"-v", "--verbose" }])
	{
		ret.sip.verbose = true;
	}
#endif
    return ret;
}

void print_usage(std::string_view binary)
{
	std::cout << "Usage: cliph [-hvudap]" << '\n';
	std::cout << "\tExample: " << binary
		<< " -u=sip:caller@domain.com -d=sip:callee@domain.com -o=sip:proxy.domain.com -a=auth_user_name -p=auth_password"
		<< std::endl;
}

}

int main(int /*argc*/, char** argv)
{
	if (auto cfg = get(argv))
	{
		cliph::controller::get().init(*cfg).run();
		//
		std::printf("Press Enter to stop...\n");
		getchar();
		std::printf("Terminating...\n");
		//
		cliph::controller::get().stop();
	}
	else
	{
		print_usage(argv[0]);
	}

	return EXIT_SUCCESS;
}