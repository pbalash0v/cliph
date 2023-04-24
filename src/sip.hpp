#ifndef cliph_sip_agent_hpp
#define cliph_sip_agent_hpp

#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <optional>
//
#include "resip/stack/SipStack.hxx"
//

namespace cliph::sip
{

struct event
{
	enum class kind
	{
		on_answer,
		on_connected,
		on_hangup
	};

	kind m_kind;
	//
	using media_ept_type = std::pair<std::string, std::uint16_t>;
	std::optional<media_ept_type> m_remote_media_ept;
	std::optional<unsigned> m_opus_pt;
};

using callback_type = std::function<void(event)>;

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
	//
	callback_type cback;
};

class ua final
{
public:
	ua(const config&);
	~ua();

	void call(std::string /*sdp*/);
	void stop() noexcept;

private:
	config m_config{};
	//
	std::thread m_agent_thr;
	std::atomic_bool m_should_stop{false};

}; //class ua

}// namespace cliph::sip

#endif // cliph_sip_agent_hpp