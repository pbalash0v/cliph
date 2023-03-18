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
#include "sip_agent.hpp"
#include "sdp.hpp"

namespace
{
cliph::sip::call_config get(char** argv)
{
	auto ret = cliph::sip::call_config{};

	auto cmdl = argh::parser{argv};
	if (auto caller_uri = cmdl({"-u", "uri"}).str(); not caller_uri.empty())
	{
		ret.from = std::move(caller_uri);
	}
	if (auto callee_uri = cmdl({"-d", "dest"}).str(); not callee_uri.empty())
	{
		ret.to = std::move(callee_uri);
	}

	if (auto auth_username = cmdl({"-a", "auth"}).str(); not auth_username.empty())
	{
		ret.auth = std::move(auth_username);
	}
	if (auto password = cmdl({"-p", "pswd"}).str(); not password.empty())
	{
		ret.pswd = std::move(password);
	}

    return ret;
}

}// namespace

int main(int /*argc*/, char** argv)
{
	cliph::audio::engine::get().init();
	cliph::sip::agent::get().run(get(argv));
	//
	std::printf("Press Enter to stop...\n");
	getchar();
	std::printf("Terminating...\n");
	//
	cliph::sip::agent::get().stop();
}