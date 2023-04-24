#ifndef cliph_controller_hpp
#define cliph_controller_hpp
//
#include <algorithm>
#include <thread>
//
#include "data_types.hpp"
#include "sound_accumulator.hpp"
#include "sound_device.hpp"
//#include "media_engine.hpp"
//#include "rtp.hpp"
#include "net.hpp"
#include "sip.hpp"
//

namespace cliph
{
struct config final
{
	//
	sound::config m_snd;
	//
	cliph::sip::config sip;
	//
	asio::ip::address local_media_ip;
};


class controller final
{
public:
	static controller& get();	
	controller& init(const cliph::config& = {});
	void run();
	void stop();

private:
	//!
	//! egress pipeline
	//! (device->acuum->encode->rtp->net)
	//!
	//! non-ts egress pipeline audio media chunks storage
	data::media_storage m_egrs_aud_storage;
	//! indexed ts slot-based accessor to media storage
	data::media_queue m_egrs_aud_q{m_egrs_aud_storage};
	//! ring buf of accessor slots
	data::media_stream m_egrs_aud_strm;
	//!
	//! inegress pipeline
	//! (net->rtp->decode->acuum->device)
	//!
	//! raw unprotected media storage
	data::media_storage m_igrs_aud_storage;
	//!
	data::media_queue m_igrs_aud_q{m_igrs_aud_storage};
	//!
	data::media_stream m_igrs_aud_strm;

private:
	//!
	std::unique_ptr<config> m_cfg_ptr;
	//!
	std::unique_ptr<sound::device> m_snd_dev_ptr;
	//!
	std::unique_ptr<sound::accumulator> m_snd_accum_ptr;
	//!
	std::unique_ptr<sip::ua> m_sip_ptr;

private:
	controller() = default;
	void sip_callback(sip::event);

};
} // namespace cliph

#endif //cliph_controller_hpp