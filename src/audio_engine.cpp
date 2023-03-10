#include <stdexcept>
#include <iostream>
#include <tuple>

//
#include <opus.h>
//
#define MINIAUDIO_IMPLEMENTATION
#include "audio_engine.hpp"

namespace
{
constexpr const auto k_sample_rate = 48000u;
constexpr const auto k_channel_count = 1;
} //namespace

namespace
{

auto* get_opus_enc()
{
    int error{};
    auto* e = opus_encoder_create(k_sample_rate, k_channel_count, OPUS_APPLICATION_VOIP, &error);
    
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



namespace  
{

auto enumerate(ma_context& context)
{
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS)
    {
        throw std::runtime_error{"Failed to initialize context"};
    }

    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_uint32 captureDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 iDevice;
    auto result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
    if (result != MA_SUCCESS) {
        throw std::runtime_error{"Failed to retrieve device information"};
    }

    printf("Playback Devices\n");
    for (iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {
        printf("    %u: %s\n", iDevice, pPlaybackDeviceInfos[iDevice].name);
    }
    printf("\n");
    printf("Capture Devices\n");
    for (iDevice = 0; iDevice < captureDeviceCount; ++iDevice) {
        printf("    %u: %s\n", iDevice, pCaptureDeviceInfos[iDevice].name);
    }

    return std::tuple{captureDeviceCount, pCaptureDeviceInfos};
}


struct user_data final
{
	OpusEncoder* opus_enc_ctx;
	ma_encoder* ma_enc;
};
auto u_d = user_data{};

void data_callback(ma_device* pDevice, void* /*pOutput*/, const void* pInput, ma_uint32 frameCount)
{
	user_data* u_d = (user_data*)pDevice->pUserData;

    [[maybe_unused]] auto* pEncoder = u_d->ma_enc;
	auto* opus_ctx = u_d->opus_enc_ctx;

    //ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
    auto packet_buff = std::array<unsigned char, 4096>{};
    if (auto len = opus_encode(opus_ctx, (const opus_int16*)pInput, frameCount, packet_buff.data(), packet_buff.size()); len < 0)
    { 
        std::cerr << get_opus_err_str(len) << '\n';
    }
    else if (len <= 2)
    {
        std::cerr << "DTX" << '\n';   
    }
    else
    {
        std::cerr << "frameCount: " << frameCount  << ", len: " << len << '\n';    
    }
}

} //anon namespace


namespace cliph::audio
{

engine& engine::get()
{
	static auto instance = engine{};
	return instance;
}

engine& engine::init()
{
	u_d.opus_enc_ctx = get_opus_enc();

	enumerate_and_select();
	
	return *this;
}

engine::~engine()
{
    ma_device_uninit(&m_device);
    ma_encoder_uninit(&m_encoder);
    ma_context_uninit(&m_context);
}

void engine::enumerate_and_select()
{
    ///
    auto [max_cap_dev_id, pCaptureDeviceInfos] = enumerate(m_context);
    while (true)
    {
        std::cin >> m_dev_id;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (m_dev_id <= max_cap_dev_id)
        {
            printf("%u: %s selected for capture\n", m_dev_id, pCaptureDeviceInfos[m_dev_id].name);
            break;
        }
        else
        {
            printf("%u: wrong selection\n", m_dev_id);
        }
    }

    auto deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = &pCaptureDeviceInfos[m_dev_id].id;
    deviceConfig.capture.format   = ma_format_s16;
    deviceConfig.capture.channels = k_channel_count;
    deviceConfig.sampleRate       = k_sample_rate;
    deviceConfig.periodSizeInMilliseconds = 20u;
    deviceConfig.dataCallback     = data_callback;
    deviceConfig.pUserData        = &u_d;

    if (auto result = ma_device_init(NULL, &deviceConfig, &m_device); result != MA_SUCCESS) {
        throw std::runtime_error{"Failed to initialize capture device"};
    }

#if 0
    auto encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, k_channel_count, k_sample_rate);

    ma_encoder encoder;
    
    if (ma_encoder_init_file(argv[1], &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize output file.\n");
        return -1;
    }
#endif
}

void engine::start()
{
    if (auto result = ma_device_start(&m_device); result != MA_SUCCESS)
    {
        throw std::runtime_error{"Failed to start device"};
    }
}

} // namespace cliph::audio
