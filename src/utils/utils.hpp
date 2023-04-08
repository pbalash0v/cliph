#ifndef cliph_utils_hpp
#define cliph_utils_hpp

//
#include "ts_mem_chunk.hpp"
#include "spsc_ts_cbuf.hpp"
#include "spsc_ts_idx_cbuf.hpp"
#include "ts_circ_buf.hpp"
#include "spsc_circ_fifo.hpp"

namespace cliph::utils
{
using raw_audio_buf = ts_mem_chunk<>;

/*struct media_element
{
	std::array<int16_t, 48*100u> raw_audio;
	std::size_t raw_audio_sz{};
	//
	std::array<uint8_t, 64u> rtp_hdr;
	std::size_t rtp_hdr_sz{};
	//
	std::array<uint8_t, 2048u> rtp_data;
	std::size_t rtp_data_sz{};
};*/


} //namespace cliph::utils

#endif // cliph_utils_hpp