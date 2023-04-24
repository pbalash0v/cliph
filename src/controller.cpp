#include <memory>
#include <stdexcept>
//
#include "asio/deferred.hpp"
#include "controller.hpp"
#include "media_engine.hpp"
#include "net.hpp"
#include "sdp.hpp"
#include "sip.hpp"
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

	m_snd_dev_ptr = std::make_unique<sound::device>(m_cfg_ptr->m_snd
		, m_egrs_aud_q, m_egrs_aud_strm, m_igrs_aud_q, m_igrs_aud_strm);
	m_snd_accum_ptr = std::make_unique<sound::accumulator>(*this, *m_cfg_ptr
		, m_egrs_aud_q, m_egrs_aud_strm, m_igrs_aud_q, m_igrs_aud_strm);

	m_cfg_ptr->sip.cback = [this] (auto kind) { sip_callback(kind); };
	m_sip_ptr = std::make_unique<sip::ua>(m_cfg_ptr->sip);

	return *this;
}

void controller::run()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }

	m_sip_ptr->call(cliph::sdp::get_local(m_cfg_ptr->local_media_ip.to_string(), m_snd_accum_ptr->port()).c_str());
}

void controller::stop()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }

	m_snd_dev_ptr->stop();
}

void controller::sip_callback(sip::event evt)
{
	switch (evt.m_kind)
	{
	case sip::event::kind::on_answer:
		{
			auto r_media_ip = asio::ip::make_address_v4(evt.m_remote_media_ept->first);
			auto r_media_port = evt.m_remote_media_ept->second;
			auto remote = asio::ip::udp::endpoint{std::move(r_media_ip), r_media_port};
			m_snd_accum_ptr->remote_media() = std::move(remote);
			m_snd_accum_ptr->run();
			m_snd_dev_ptr->run();
		}
		break;
	case sip::event::kind::on_connected:
		break;
	case sip::event::kind::on_hangup:
		m_snd_dev_ptr->stop();
		break;		
	default:
		throw std::runtime_error{"unhandled event"};
	}
}

} // namespace cliph
