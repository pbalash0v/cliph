#include "controller.hpp"
#include <memory>
#include <stdexcept>

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
	m_cfg_ptr = std::make_unique<config>(cfg);
	m_snd_eng_ptr = std::make_unique<sound::engine>(m_cfg_ptr->m_snd, m_capt_cbuf, m_playb_cbuf);
	m_snd_accum_ptr = std::make_unique<sound::accumulator>(*this, m_capt_cbuf);

	return *this;
}

void controller::run()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }
	m_snd_accum_ptr->run();
	m_snd_eng_ptr->run();
}

void controller::stop()
{
	if (!m_cfg_ptr) { throw std::runtime_error{"Uninitialized"}; }
	m_snd_eng_ptr->stop();
}

std::string controller::description() const
{
	//return cliph::sdp::get_local(local_media.to_string(), sock_ptr->port()).c_str();
	return {};
}

} // namespace cliph
