#ifndef cliph_sip_agent_hpp
#define cliph_sip_agent_hpp

#include <functional>
#include <thread>
#include <atomic>
#include <optional>
//
#include "resip/stack/SipStack.hxx"
//
#include "controller.hpp"


namespace cliph::sip
{

struct transport_config
{
	resip::TransportType transport{resip::TransportType::UDP};
	std::uint16_t port = 0;
};

struct config
{
	std::string from{"sip:caller@localhost"};
	std::string to{"sip:callee@localhost"};
	std::optional<std::string> outbound_prx;
	std::string auth;
	std::string pswd;
	bool verbose{false};
	transport_config tp_cfg;
};

class ua final
{
public:
	ua(const config&);
	~ua();

	void call(std::string);
	void stop() noexcept;

private:
	config m_config{};
	std::thread m_agent_thr;
	std::atomic_bool m_should_stop{false};

}; //class ua

}// namespace cliph::sip

#endif // cliph_sip_agent_hpp