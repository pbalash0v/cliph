#ifndef sound_accumulator_hpp
#define sound_accumulator_hpp

#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
//
//
#include "asio/io_context.hpp"
#include "asio/ip/address.hpp"
#include "asio/ip/udp.hpp"
#include "asio.hpp"
//
//
#include "rtp_stream.hpp"
#include "media_engine.hpp"
#include "sound_device.hpp"
#include "data_types.hpp"

namespace cliph
{
class controller;
struct config;
}

namespace cliph::sound
{

class accumulator final
{
public:	
	accumulator(cliph::controller& controller
		, const cliph::config& snd_cfg
		, data::raw_audio_buf& capt_buf
		, data::raw_audio_buf& playb_buf
		, data::media_queue& audio_q
		, data::media_buf& audio_buf);

	~accumulator();

public:		
	void run();

private:
	cliph::controller& m_controller;
	//
	const cliph::sound::config& m_snd_cfg;
	//
	data::raw_audio_buf& m_capt_snd_cbuf;
	//
	data::raw_audio_buf& m_playb_snd_cbuf;

private:
	std::thread m_egress_thr;
	std::thread m_ingress_thr;
	//
	std::array<std::int16_t, 8192u> m_egress_snd_accum{};
	std::size_t m_egress_snd_offset{};
	//
	std::array<std::int16_t, 8192u> m_inress_snd_accum{};
	std::size_t m_inress_snd_offset{};
	//
	data::media_queue& m_audio_q;
	//
	data::media_buf& m_audio_buf;
	//
	std::chrono::milliseconds m_next_len{20};
	//
	OpusEncoder* m_opus_enc_ctx{};
	OpusDecoder* m_opus_dec_ctx{};
	//
	rtpp::stream m_strm;
	//
	asio::io_context m_io;
	asio::ip::udp::socket m_socket;
	asio::ip::address m_local_media_ip;
	asio::ip::udp::endpoint m_remote_media;

private:	
	void egress_loop();

};

} // namespace cliph::sound

#endif // sound_accumulator_hpp
