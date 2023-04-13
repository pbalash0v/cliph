#include <memory>
#include <stdexcept>
//
#include "controller.hpp"
#include "media_engine.hpp"
#include "net.hpp"
#include "sound_device.hpp"


namespace
{

} //namespace


namespace cliph
{

controller& controller::get()
{
	static auto instance = controller{};
	return instance;
}

controller& controller::init(const cliph::config& cfg)
{
	if (m_cfg_ptr) { throw std::runtime_error{"Already initialized"}; }

	m_cfg_ptr = std::make_unique<config>(cfg);
	m_snd_dev_ptr = std::make_unique<sound::device>(m_cfg_ptr->m_snd, m_capt_cbuf, m_playb_cbuf);
	m_snd_accum_ptr = std::make_unique<sound::accumulator>(*this
		, *m_cfg_ptr, m_capt_cbuf, m_playb_cbuf, m_egress_audio_q, m_egress_audio_buf);
	//m_audio_media_ptr = std::make_unique<media::audio>(media::audio::config{m_cfg_ptr->m_snd}
		//, m_egress_audio_buf, m_egress_audio_rtp_buf);
	//m_audio_rtp_ptr = std::make_unique<rtp::engine>(m_egress_audio_rtp_buf, m_egress_net_audio_buf);
	//m_net_ptr = std::make_unique<net::engine>(net::config{}
		//, m_egress_net_audio_buf, m_ingress_audio_q, m_ingress_audio_rtp_buf);

	return *this;
}

void controller::run()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }

	//m_audio_rtp_ptr->run();
	//m_audio_media_ptr->run();
	m_snd_accum_ptr->run();
	m_snd_dev_ptr->run();
}

void controller::stop()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }

	m_snd_dev_ptr->stop();
}

std::string controller::description() const
{
	//return cliph::sdp::get_local(local_media.to_string(), sock_ptr->port()).c_str();
	return {};
}

} // namespace cliph
