#ifndef cliph_controller_hpp
#define cliph_controller_hpp
//
#include <thread>
//
#include "media_engine.hpp"
#include "audio_engine.hpp"
#include "sip_agent.hpp"
#include "net.hpp"
//
namespace cliph
{
struct config final
{
	inline static constexpr const auto k_rtp_advance_interval = rtpp::stream::duration_type{kPeriodSizeInMilliseconds};

	cliph::sip::config sip;
	cliph::sound::config sound;
	asio::ip::address media_ip;
};

class controller final
{
public:
	static controller& get() noexcept;

public:
	~controller();

public:
	controller& init(const config&);
	 controller& get() noexcept;

	controller& set_net_sink(std::string_view, std::uint16_t);
	controller& set_remote_opus_params(std::uint8_t);
	//
	std::string description() const;

private:
	utils::audio_circ_buf m_capt_buf;
	utils::audio_circ_buf m_playb_cbuf;

private:
	//
	config m_cfg;
	//
	sound::engine m_sound_engine{m_cfg.sound, m_capt_buf, m_playb_cbuf};
	//
	media::audio m_audio{{m_cfg.sound}, m_capt_buf};

	rtpp::stream rtp_stream;

private:	
	controller() = default;
};
} // namespace cliph

#endif //cliph_controller_hpp