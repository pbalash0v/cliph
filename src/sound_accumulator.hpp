#ifndef sound_accumulator_hpp
#define sound_accumulator_hpp

#include <cstddef>
#include <thread>
#include <vector>
//
#include "media_engine.hpp"
#include "sound_engine.hpp"
#include "data_types.hpp"

namespace cliph
{
class controller;
}

namespace cliph::sound
{

class accumulator final
{
public:	
	accumulator(cliph::controller& controller
		, const cliph::sound::config& snd_cfg
		, data::raw_audio_buf& capt_buf
		, data::raw_audio_buf& playb_buf
		, data::media_queue& audio_q
		, data::media_buf& audio_buf)
		: m_controller{controller}
		, m_snd_cfg{snd_cfg}
		, m_capt_snd_cbuf{capt_buf}
		, m_playb_snd_cbuf{playb_buf}
		, m_audio_q{audio_q}
		, m_audio_buf{audio_buf}
	{
	}

	~accumulator();

public:		
	void run();

private:
	cliph::controller& m_controller;
	//
	const cliph::sound::config& m_snd_cfg;
	//
	data::raw_audio_buf& m_capt_snd_cbuf;
	//
	data::raw_audio_buf& m_playb_snd_cbuf;

private:
	std::thread m_egress_thr;
	std::thread m_ingress_thr;
	//
	std::array<std::byte, 8192u> m_egress_snd_accum{};
	std::size_t m_egress_snd_offset{};
	//
	std::array<std::byte, 8192u> m_inress_snd_accum{};
	std::size_t m_inress_snd_offset{};
	//
	data::media_queue& m_audio_q;
	//
	data::media_buf& m_audio_buf;
	//
	std::chrono::milliseconds m_next_len{20};

private:	
	void egress_loop();

};

} // namespace cliph::sound

#endif // sound_accumulator_hpp
