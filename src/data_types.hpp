#ifndef data_types_hpp
#define data_types_hpp

#include <chrono>
//
#include "ts_idx_q.hpp"
#include "ts_mem_chunk.hpp"

namespace cliph::data
{

using raw_audio_buf = cliph::utils::ts_mem_chunk<>;
//
struct media
{
//
	std::array<int16_t, 8192u> raw_audio;
	std::size_t raw_audio_sz{};
	std::chrono::milliseconds raw_audio_len{};
	//
	std::array<uint8_t, 64u> rtp_hdr{};
	std::size_t rtp_hdr_sz{};
	//
	std::array<uint8_t, 2048u> rtp_data{};
	std::size_t rtp_data_sz{};
//
	void reset()
	{
		raw_audio_sz = {};
		raw_audio_len = {};
		rtp_hdr_sz = {};
		rtp_data_sz = {};
	}
};

using media_stream = std::array<media, 64u>;
using media_queue = utils::ts_idx_q<media_stream>;
using slot_type = media_queue::slot_type;
using media_buf = utils::ts_cbuf<slot_type>;

} //namespace cliph::utils

#endif // data_types_hpp