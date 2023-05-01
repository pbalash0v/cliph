#ifndef sound_accumulator_hpp
#define sound_accumulator_hpp

#include <chrono>
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
		, data::media_queue&
		, data::media_stream&
		, data::media_queue&
		, data::media_stream&);		

	~accumulator();

public:		
	void run();
	auto port()
	{
		return m_socket.local_endpoint().port();
	}

	auto& remote_media()
	{
		return m_remote_media;
	}


private:
	cliph::controller& m_controller;
	//
	const cliph::sound::config& m_snd_cfg;
	//
	data::media_queue& m_out_q;
	data::media_stream& m_out_strm;
	//
	data::media_queue& m_in_q;
	data::media_stream& m_in_strm;	

private:
	std::thread m_egress_thr;
	std::thread m_ingress_thr;
	//
	std::array<std::int16_t, 8192u> m_egress_snd_accum{};
	std::size_t m_egress_sample_count{};
	std::size_t m_egress_bytes_count{};
	std::chrono::milliseconds m_egress_snd_sz{};
	//
	std::array<std::int16_t, 8192u> m_ingress_snd_accum{};
	std::size_t m_ingress_snd_offset{};
	//
	std::chrono::milliseconds m_next_len{10};
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
	void egress_loop(bool debug = true);
	void ingress_loop(bool debug = false);

};

} // namespace cliph::sound

#endif // sound_accumulator_hpp
