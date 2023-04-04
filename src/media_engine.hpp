#ifndef cliph_media_engine_hpp
#define cliph_media_engine_hpp
//
#include <thread>
//
#include <opus.h>
//
#include "audio_engine.hpp"
#include "utils.hpp"

namespace cliph::media
{

class audio final
{
public:
	struct config
	{
		const sound::config& sound_cfg;
		inline static constexpr const auto k_opus_dyn_pt = 96u;
		inline static constexpr const auto k_opus_rtp_clock = 48000u;		
	};

public:
	audio(const cliph::media::audio::config&, utils::audio_circ_buf&);
	~audio();

public:
	void opus_decoder_params(std::uint8_t pt);

private:
	utils::audio_circ_buf& m_raw_audio_circ_buf;

private:
	std::thread m_audio_thr;
	utils::audio_circ_buf::chunk_type m_audio_chunk;
	//
	OpusEncoder* m_opus_enc_ctx{};
	OpusDecoder* m_opus_dec_ctx{};
private:
	void loop();
};
} // namespace cliph

#endif //cliph_media_engine_hpp