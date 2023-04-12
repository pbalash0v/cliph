#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ratio>
#include <thread>
//
#include "sound_accumulator.hpp"
#include "controller.hpp"

//

using namespace std::chrono;
using namespace std::chrono_literals;

namespace cliph::sound
{

accumulator::~accumulator()	
{
	if (m_egress_thr.joinable()) { m_egress_thr.join(); }
	if (m_ingress_thr.joinable()) { m_ingress_thr.join(); }
}

void accumulator::run()
{
	m_egress_thr = std::thread{[this]()
	{
		std::cerr << "accumulator egress_loop started\n";
		egress_loop();
		std::cerr << "accumulator egress_loop finished\n";
	}};

	if (auto rc = ::pthread_setname_np(m_egress_thr.native_handle(), "EGRS_SND_ACCUM"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void accumulator::egress_loop()
{
	// To encode a frame opus_encode() must be called with exactly one frame (2.5, 5, 10, 20, 40 or 60 ms) of audio data
	const auto k_lengths = std::array<milliseconds, 4u>{10ms, 20ms, 40ms, 60ms};
	while (true)
	{
		if (auto sz = m_capt_snd_cbuf.get(m_egress_snd_accum.data() + m_egress_snd_offset, m_egress_snd_accum.size()))
		{
			m_egress_snd_offset += sz;
			//
			const auto samples_per_ms = m_snd_cfg.m_audio_device_sample_rate/1000u;
			const auto curr_accum = milliseconds{(m_egress_snd_offset/sizeof(int16_t))/samples_per_ms};
			assert(milliseconds{(sz/sizeof(int16_t))/samples_per_ms} == 10ms);
			if (curr_accum < m_next_len) { continue; }

			//
			for (auto slot = m_audio_q.get();; slot = m_audio_q.get())
			{
				if (!slot) continue;
				slot->reset();
				slot->raw_audio_len = curr_accum;
				slot->raw_audio_sz = m_egress_snd_offset/sizeof(int16_t);
				std::memcpy(slot->raw_audio.data(), m_egress_snd_accum.data(), m_egress_snd_offset);
				std::cerr << "slot filled: " << curr_accum.count() << "ms, len: " << m_egress_snd_offset/2 << '\n';
				//
				m_audio_buf.put(std::move(slot));
				break;
			}
			// Reset and send to media processing
			m_egress_snd_offset = 0;
			m_next_len = k_lengths[std::rand() % k_lengths.size()];
		}
		else // shutdown pipeline
		{
			for (auto slot = m_audio_q.get();; slot = m_audio_q.get())
			{
				if (!slot) continue;
				slot->raw_audio_len = 0ms;
				slot->raw_audio_sz = 0;
				m_audio_buf.put(std::move(slot));
				break;
			}
			break;
		}
	}
}


} //namespace cliph::sound