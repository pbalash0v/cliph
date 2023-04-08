#ifndef sound_accumulator_hpp
#define sound_accumulator_hpp

#include <thread>
//
#include "sound_engine.hpp"
#include "utils.hpp"

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
		, utils::raw_audio_buf& buf)
		: m_controller{controller}
		, m_snd_cbuf(buf)
	{
	}

	~accumulator();

public:		
	void run();

private:
	cliph::controller& m_controller;
	utils::raw_audio_buf& m_snd_cbuf;

private:
	std::thread m_thr;
	std::array<std::byte, 4096u> m_raw_snd_accum{};
	std::size_t m_snd_offset{};

private:	
	void loop();

};

} // namespace cliph::sound

#endif // sound_accumulator_hpp
