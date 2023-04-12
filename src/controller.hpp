#ifndef cliph_controller_hpp
#define cliph_controller_hpp
//
#include <algorithm>
#include <thread>
//


#include "sound_accumulator.hpp"
#include "sound_device.hpp"
#include "media_engine.hpp"
#include "rtp.hpp"
#include "net.hpp"
//#include "sip.hpp"

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
	//! sound source capture queue
	//! (raw samples from device)
	data::raw_audio_buf m_capt_cbuf;
	//! sound source palyback queue
	//! (raw samples to device)
	data::raw_audio_buf m_playb_cbuf;
	//!
	//! egress pipeline
	//! (device->acuum->encode->rtp->net)
	//!
	//! non-ts egress pipeline audio media chunks storage
	data::media_stream m_egress_audio_strm;
	//! indexed ts slot-based accessor to media storage
	data::media_queue m_egress_audio_q{m_egress_audio_strm};
	//!
	data::media_buf m_egress_audio_buf;
	//!
	data::media_buf m_egress_audio_rtp_buf;
	//!
	data::media_buf m_egress_net_audio_buf;
	//!
	//! inegress pipeline
	//! (net->rtp->decode->acuum->device)
	//!
	//! raw unprotected media storage
	data::media_stream m_ingress_audio_strm;
	//!
	data::media_queue m_ingress_audio_q{m_ingress_audio_strm};
	//!
	data::media_buf m_ingress_audio_rtp_buf;
	//!
	data::media_buf m_ingress_audio_buf;
	//!

private:
	//!
	std::unique_ptr<config> m_cfg_ptr;
	//!
	std::unique_ptr<sound::device> m_snd_dev_ptr;
	//!
	std::unique_ptr<sound::accumulator> m_snd_accum_ptr;
	//!
	std::unique_ptr<media::audio> m_audio_media_ptr;
	//!
	std::unique_ptr<rtp::engine> m_audio_rtp_ptr;
	//!
	std::unique_ptr<net::engine> m_net_ptr;

private:
	controller() = default;

};
} // namespace cliph

#endif //cliph_controller_hpp