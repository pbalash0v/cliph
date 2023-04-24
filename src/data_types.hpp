#ifndef data_types_hpp
#define data_types_hpp

#include <chrono>
#include <array>
#include <cstdint>
//
#include "ts_idx_q.hpp"
#include "ts_mem_chunk.hpp"

namespace cliph::data
{

using raw_audio_buf = cliph::utils::ts_mem_chunk<>;
//
//
//
struct media_elt
{
	//
	//
	//
	struct sound
	{
		using sample_type = std::int16_t;
		constexpr static const auto k_capacity = 2048u;
		//
		std::array<sample_type, k_capacity> data{};
		std::size_t sample_count{};
		std::chrono::milliseconds sz{};
		unsigned sample_rate{};
		//
		auto bytes_sz() const noexcept
		{
			return sample_count*sizeof(sample_type);
		}

	};
	sound m_sound;
	//
	//
	//
	struct rtp
	{
		// TODO: check RFC
		constexpr static const auto k_hdr_capacity = 64u;
		//
		std::array<uint8_t, k_hdr_capacity> hdr{};
		std::size_t hdr_sz{};
		//
		constexpr static const auto k_data_capacity = 2048u;
		//
		std::array<uint8_t, k_data_capacity> data{};
		std::size_t data_sz{};
	};
	rtp m_rtp;
	//
	//
	//
	void reset()
	{
		m_sound.sample_count = {};
		m_sound.sz = {};
		m_sound.sample_rate = {};
		//
		m_rtp.hdr_sz = {};
		m_rtp.data_sz = {};
	}
};

constexpr const auto media_storage_sz = 64u;
using media_storage = std::array<media_elt, media_storage_sz>;
//
using media_queue = utils::ts_idx_queue<media_storage>;
using slot_type = media_queue::slot_type;
//
using media_stream = utils::ts_cbuf<slot_type>;

} //namespace cliph::utils

#endif // data_types_hpp