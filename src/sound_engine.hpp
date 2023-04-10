#ifndef sound_engine_hpp
#define sound_engine_hpp

//
#include <chrono>
//
#include <miniaudio.h>
//
#include "data_types.hpp"

namespace cliph::sound
{
using namespace cliph;

struct config
{
	std::chrono::milliseconds m_period_sz{10u};
	unsigned m_audio_device_sample_rate{48000u};
	bool m_audio_device_stereo_capture{false};
	bool m_audio_device_stereo_playback{false};
};

class engine final
{
public:
	explicit engine(const sound::config&, data::raw_audio_buf&, data::raw_audio_buf&);
	~engine();

public:
	//
	void run();
	void pause();
	void stop();

private:
	const sound::config& m_cfg;
	data::raw_audio_buf& m_cpt_buf;
	data::raw_audio_buf& m_plb_buf;

private:
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	void enumerate_and_select();
};

} // namespace cliph::sound

#endif // sound_engine_hpp