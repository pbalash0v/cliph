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

class device final
{
public:
	explicit device(const sound::config&
		, data::media_queue&, data::media_stream&
		, data::media_queue&, data::media_stream&);
	~device();

public:
	//
	void run();
	void pause();
	void stop();

private:
	//
	const sound::config& m_cfg;
	//
	data::media_queue& m_cpt_q;
	data::media_stream& m_cpt_strm;
	//
	data::media_queue& m_plb_q;
	data::media_stream& m_plb_strm;

private:
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	void enumerate_and_select();
};

} // namespace cliph::sound

#endif // sound_engine_hpp