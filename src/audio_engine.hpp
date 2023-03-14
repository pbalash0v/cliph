#ifndef audio_engine_hpp
#define audio_engine_hpp

//
#include <miniaudio.h>
//

namespace cliph::audio
{

class engine final
{
public:
	static engine& get() noexcept;	
public:
	~engine();
	engine& init();
	void start();
	void stop();

private:
	unsigned m_dev_id{};
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	engine() = default;
	void enumerate_and_select();
};

} // namespace cliph::audio

#endif // audio_engine_hpp