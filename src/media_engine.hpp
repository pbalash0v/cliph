#ifndef cliph_media_engine_hpp
#define cliph_media_engine_hpp
//
#include <thread>
//
#include <opus.h>
//
#include "data_types.hpp"
#include "sound_device.hpp"

namespace cliph::media
{

class audio final
{
public:
	struct config
	{
		const sound::config& sound_cfg;
		bool dtx{false};
		//inline static constexpr const auto k_opus_dyn_pt = 96u;
		//inline static constexpr const auto k_opus_rtp_clock = 48000u;
	};

public:
	audio(const audio::config&
		, data::media_queue&
		, data::media_stream&
		, data::media_queue&
		, data::media_stream&);
	~audio();

public:
	void run();
	//void opus_decoder_params(std::uint8_t pt);

private:
	data::media_queue& m_out_q;
	data::media_stream& m_cpt_strm;

	data::media_queue& m_in_q;
	data::media_stream& m_in_strm;

private:
	std::thread m_thr;
	//
	OpusEncoder* m_opus_enc_ctx{};
	OpusDecoder* m_opus_dec_ctx{};

private:
	void egress_loop();
};
} // namespace cliph

#endif //cliph_media_engine_hpp