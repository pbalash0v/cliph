#include <iostream>
//

//
#include "media_engine.hpp"


namespace
{

auto* get_opus_enc()
{
    int error{};
    auto* e = opus_encoder_create(k_audio_device_sample_rate, k_audio_device_channel_count, OPUS_APPLICATION_VOIP, &error);
    
    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

    if (auto ret = opus_encoder_ctl(e, OPUS_SET_DTX(1)); ret != OPUS_OK)
    {
       throw std::runtime_error{"OPUS_SET_DTX failed"};
    }

    return e;
}

auto* get_opus_dec()
{
    int error{};
    auto* e = opus_decoder_create(k_audio_device_sample_rate, k_audio_device_channel_count, &error);
    
    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

    return e;
}

std::string_view get_opus_err_str(opus_int32 err)
{
    switch (err)
    {
    case OPUS_OK: return "OPUS_OK";
    case OPUS_BAD_ARG: return "OPUS_BAD_ARG";        
    case OPUS_BUFFER_TOO_SMALL: return "OPUS_BUFFER_TOO_SMALL";        
    case OPUS_INTERNAL_ERROR: return "OPUS_INTERNAL_ERROR";
    case OPUS_INVALID_PACKET: return "OPUS_INVALID_PACKET";
    case OPUS_UNIMPLEMENTED: return "OPUS_UNIMPLEMENTED";
    case OPUS_INVALID_STATE: return "OPUS_INVALID_STATE";
    case OPUS_ALLOC_FAIL: return "OPUS_ALLOC_FAIL";
    default: return "OPUS_UNKNOWN";
    }
}

} // anon namespace




namespace cliph::media
{
audio::audio(const audio::config& cfg, utils::audio_circ_buf& acb)
	: m_raw_audio_circ_buf{acb}
	, m_opus_enc_ctx{get_opus_enc()}
	, m_opus_dec_ctx{get_opus_dec()}
{
	m_audio_thr = std::thread{[this]
	{
		loop();
	}};
}

audio::~audio()
{
	m_audio_thr.join();
	::opus_encoder_destroy(m_opus_enc_ctx);
	::opus_decoder_destroy(m_opus_dec_ctx);	
}

void audio::loop()
{
	auto encode_buff = std::array<unsigned char, 4096>{};

	auto* new_raw_audio_chunk = std::get<0>(m_audio_chunk).data();
	while (true)
	{
		auto len = m_raw_audio_circ_buf.get(new_raw_audio_chunk);
		if (len == 0) { break; }

		//
		if (auto encoded = ::opus_encode(m_opus_enc_ctx
			, static_cast<const opus_int16*>(new_raw_audio_chunk), len
			, encode_buff.data(), encode_buff.size()); encoded < 0)
		{
			std::cerr << get_opus_err_str(encoded) << '\n';
		}
		else
		{
			while (!out_rtp_buf->put(encode_buff.data(), encoded))

			if (encoded < 3) { continue; } // DTX 
		}
	}

	std::cerr << "audio_media_loop over\n";
}

} // namespace cliph
