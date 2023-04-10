#ifndef cliph_controller_hpp
#define cliph_controller_hpp
//
#include <algorithm>
#include <thread>
//

//#include "sip_agent.hpp"
//#include "net.hpp"
#include "sound_accumulator.hpp"
#include "sound_engine.hpp"
#include "media_engine.hpp"
//

namespace cliph
{
struct config final
{
	//cliph::sip::config sip;
	sound::config m_snd;
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

private:
	//! sound source input queue (raw samples)
	data::raw_audio_buf m_capt_cbuf;
	//! sound source output queue (raw samples)
	data::raw_audio_buf m_playb_cbuf;
	//!
	//! non-ts egress pipeline audio media chunks storage
	data::media_stream m_egress_audio_strm;
	//! indexed ts slot-based accesser to media storage
	data::media_queue m_egress_audio_q{m_egress_audio_strm};
	//!
	data::media_buf m_egress_audio_buf;

	//!
	//! raw unprotected media storage
	data::media_stream m_ingress_audio_strm;
	data::media_queue m_ingress_audio_q{m_ingress_audio_strm};

private:
	//
	std::unique_ptr<config> m_cfg_ptr;
	//
	std::unique_ptr<sound::engine> m_snd_eng_ptr;
	//
	std::unique_ptr<sound::accumulator> m_snd_accum_ptr;
	//
	std::unique_ptr<media::audio> m_audio_media_ptr;
	//

private:
	controller() = default;

};
} // namespace cliph

#endif //cliph_controller_hpp