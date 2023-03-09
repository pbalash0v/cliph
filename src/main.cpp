#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <atomic>

#include "UAC.hpp"

namespace
{

void data_callback(ma_device* pDevice, void* /*pOutput*/, const void* pInput, ma_uint32 frameCount)
{
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;
    MA_ASSERT(pEncoder != NULL);

    ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);
}


auto enumerate(ma_context& context)
{
    ma_result result;
    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_uint32 captureDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 iDevice;

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        throw std::runtime_error{"Failed to initialize context"};
    }

    result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
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

} // anon namespace


int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("No output file.\n");
        return -1;
    }

    ///
    ma_context context;
    auto [max_cap_dev_id, pCaptureDeviceInfos] = enumerate(context);
    auto dev_id = 0u;
    while (true)
    {
        std::cin >> dev_id;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (dev_id > 0 and dev_id <= max_cap_dev_id)
        {
            printf("%u: %s selected\n", dev_id, pCaptureDeviceInfos[dev_id].name);
            break;
        }
        else
        {
            printf("%u: wrong selection\n", dev_id);
        }
    }


    // now record
    ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, 1/*2*/, 8000/*44100*/);

    ma_encoder encoder;
    if (ma_encoder_init_file(argv[1], &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize output file.\n");
        return -1;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = &pCaptureDeviceInfos[dev_id].id;
    deviceConfig.capture.format   = encoder.config.format;
    deviceConfig.capture.channels = encoder.config.channels;
    deviceConfig.sampleRate       = encoder.config.sampleRate;
    deviceConfig.dataCallback     = data_callback;
    deviceConfig.pUserData        = &encoder;

    ma_device device;
    if (auto result = ma_device_init(NULL, &deviceConfig, &device); result != MA_SUCCESS) {
        throw std::runtime_error{"Failed to initialize capture device"};
    }
    if (auto result = ma_device_start(&device); result != MA_SUCCESS) {
        ma_device_uninit(&device);
        throw std::runtime_error{"Failed to start device"};
    }

    //
    auto stop = std::atomic_bool{false};
    auto uac_thread = std::thread{[&]()
    {
        run_UAC(stop, "callee", resip::TransportType::UDP);
        std::printf("UAC thread ended\n");
    }};

    std::printf("Press Enter to stop...\n");
    getchar();
    std::printf("Terminating...\n");
    stop = true;

    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder);
    ma_context_uninit(&context);

    uac_thread.join();

    return 0;
}