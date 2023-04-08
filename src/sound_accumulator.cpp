#include <thread>
//
#include "sound_accumulator.hpp"
#include "controller.hpp"

//

namespace cliph::sound
{

accumulator::~accumulator()	
{
	if (m_thr.joinable()) m_thr.join();
}

void accumulator::run()
{
	m_thr = std::thread{[this]()
	{
		loop();
	}};
}

void accumulator::loop()
{
	while (true)
	{
		if (auto sz = m_snd_cbuf.get(m_raw_snd_accum.data() + m_snd_offset, m_raw_snd_accum.size()))
		{
			//m_snd_offset += sz;

/*			if (m_controller.get().sound_frames_count() == m_snd_offset)
			{

			}*/

		}
		else
		{
			break;
		}
	}
}


} //namespace cliph::sound