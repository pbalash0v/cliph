#include <cstdint>
#include <iostream>
#include <string>
//
#include "data_types.hpp"
#include "media_engine.hpp"


namespace
{

auto* get_opus_enc(std::uint32_t sample_rate, uint8_t channel_count, bool dtx)
{
	auto error = int{};
    auto* e = opus_encoder_create(sample_rate, channel_count, OPUS_APPLICATION_VOIP, &error);
    
	if (error != OPUS_OK)
	{
		throw std::runtime_error{"opus_encoder_create failed"};
	}

    if (dtx)
    {
		if (auto ret = opus_encoder_ctl(e, OPUS_SET_DTX(1u)); ret != OPUS_OK)
		{
			throw std::runtime_error{"OPUS_SET_DTX failed"};
		}
	}

	return e;
}

auto* get_opus_dec(std::uint32_t sample_rate, uint8_t channel_count)
{
	auto error = int{};
	auto* d = opus_decoder_create(sample_rate, channel_count, &error);
    
    if (error != OPUS_OK)
    {
        throw std::runtime_error{"opus_encoder_create failed"};
    }

	return d;
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
audio::audio(const audio::config& cfg
	, data::media_buf& in_buf
	, data::media_buf& out_buf)
	: m_in_buf{in_buf}
	, m_out_buf{out_buf}
	, m_opus_enc_ctx{get_opus_enc(cfg.sound_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u, cfg.dtx)}
	, m_opus_dec_ctx{get_opus_dec(cfg.sound_cfg.m_audio_device_sample_rate, /*channel_count*/ 1u)}
{
}

audio::~audio()
{
	if (m_thr.joinable()) { m_thr.join(); }
	::opus_encoder_destroy(m_opus_enc_ctx);
	::opus_decoder_destroy(m_opus_dec_ctx);	
}

void audio::run()
{
	m_thr = std::thread{[this]
	{
		std::cerr << "audio_media egress loop started\n";
		egress_loop();
		std::cerr << "audio_media egress loop finished\n";
	}};
	if (auto rc = ::pthread_setname_np(m_thr.native_handle(), "EGRS_MED_ENG"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
}

void audio::egress_loop()
{
	while (true)
	{
		auto slot = data::slot_type{};
		m_in_buf.get(slot);
		//
		if (slot->raw_audio_sz == 0)
		{
			m_out_buf.put(std::move(slot));
			break;
		}

		//
		const auto& raw_audio_data = slot->raw_audio.data();
		const auto& raw_audio_data_sz = slot->raw_audio_sz;
		auto* const encoded_audio_data = slot->rtp_data.data();
		const auto encoded_audio_data_len = slot->rtp_data.size();
		if (auto encoded = ::opus_encode(m_opus_enc_ctx
			, static_cast<const opus_int16*>(raw_audio_data), raw_audio_data_sz
			, encoded_audio_data, encoded_audio_data_len)
			; encoded < 0)
		{
			std::cerr << get_opus_err_str(encoded) << '\n';
		}
		else
		{
			if (encoded < 3) // DTX 
			{
				std::cerr << "DTX\n";
			}
			else
			{
				slot->rtp_data_sz = encoded;
				std::cerr << std::string{"encoded: "} + std::to_string(encoded) + "\n";	
			}
		}

		m_out_buf.put(std::move(slot));
	}
}

} // namespace cliph
