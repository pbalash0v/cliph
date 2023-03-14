#ifndef cliph_sip_agent_hpp
#define cliph_sip_agent_hpp

#include <functional>
#include <thread>
#include <atomic>
//
#include "resip/stack/SipStack.hxx"


namespace cliph::sip
{

struct config
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

class agent final
{
public:
	using on_answer_cback_type = std::function<void(void)>;
	using on_hangup_cback_type = std::function<void(void)>;

public:
	static agent& get() noexcept;
	void run(on_answer_cback_type&&, on_hangup_cback_type&&);
	void stop() noexcept;
	~agent();

private:
	agent() = default;

private:
	config m_config{};
	call_config m_call_config{};
	std::thread m_agent_thread;
	std::atomic_bool m_should_stop{false};

}; //class agent

}// namespace cliph::sip

#endif // cliph_sip_agent_hpp