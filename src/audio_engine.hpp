#ifndef audio_engine_hpp
#define audio_engine_hpp

//
#include <chrono>
#include <string_view>
#include <cstdint>
//
#include <miniaudio.h>
//
#include "utils.hpp"

namespace cliph::sound
{

struct config
{
	std::chrono::milliseconds m_period_sz{20u};
	unsigned m_audio_device_sample_rate{48000u};
	bool m_audio_device_stereo_capture{};
	bool m_audio_device_stereo_playback{};
};

class engine final
{
public:
	explicit engine(const cliph::sound::config&
		, utils::audio_circ_buf&, utils::audio_circ_buf&);

public:
	//
	void start();
	void pause();
	void stop();

public:
	~engine();

private:
	cliph::sound::config m_cfg;
	utils::audio_circ_buf& m_capt_buf;
	utils::audio_circ_buf& m_playb_buf;
private:
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	void enumerate_and_select();
};

} // namespace cliph::audio

#endif // audio_engine_hpp