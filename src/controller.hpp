#ifndef cliph_controller_hpp
#define cliph_controller_hpp
//
#include <algorithm>
#include <thread>
//
//#include "media_engine.hpp"
//#include "sip_agent.hpp"
//#include "net.hpp"
#include "sound_accumulator.hpp"
#include "sound_engine.hpp"
//
namespace cliph
{
struct config final
{
	//inline static constexpr const auto k_rtp_advance_interval = rtpp::stream::duration_type{kPeriodSizeInMilliseconds};

	//cliph::sip::config sip;
	sound::config m_snd{};
	//asio::ip::address media_ip;
};


class controller final
{
public:
	static controller& get();	
	controller& init(const cliph::config& = {});
	void run();
	void stop();

public:
	//controller& set_net_sink(std::string_view, std::uint16_t);
	controller& set_remote_sd(std::uint8_t);
	//
	std::string description() const;
	//

private:
	//
	utils::raw_audio_buf m_capt_cbuf;
	utils::raw_audio_buf m_playb_cbuf;
	//
private:
	//
	std::unique_ptr<config> m_cfg_ptr;
	//
	std::unique_ptr<sound::engine> m_snd_eng_ptr;
	//
	std::unique_ptr<sound::accumulator> m_snd_accum_ptr;
	//media::audio m_audio{{m_cfg.sound}, m_capt_buf};

	//rtpp::stream rtp_stream;

private:
	
	controller() = default;

};
} // namespace cliph

#endif //cliph_controller_hpp