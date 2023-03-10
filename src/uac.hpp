#ifndef basic_invite_UAC_hpp
#define basic_invite_UAC_hpp

#include <atomic>
#include <string>
//
#include "resip/stack/SipStack.hxx"


namespace cliph::sip
{

struct agent_config
{
	resip::TransportType transport{resip::TransportType::UDP};
	std::uint16_t port = 0;
};

struct call_config
{
	std::string from_user{"caller"};
	std::string from_domain{"localhost"};
	std::string to_user{"callee"};
	std::string to_domain{"localhost"};
};

void run(std::atomic_bool& should_stop, call_config = {}, agent_config = {});

} // namespace cliph::sip

#endif // basic_invite_UAC_hpp
