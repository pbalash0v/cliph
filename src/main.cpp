#include <stdio.h>
#include <stdlib.h>
//
#include <ios>
#include <iostream>
#include <atomic>
//
#include "audio_engine.hpp"
#include "uac.hpp"
#include "sdp.hpp"


int main(int argc, char** argv)
{
    cliph::audio::engine::get().init();
    //
    auto should_stop = std::atomic_bool{false};
    auto uac_thread = std::thread{[&]()
    {
        //cliph::sip::run(should_stop);
        std::printf("sip agent thread ended\n");
    }};
    //cliph::audio::engine::get().start();
    //
    std::printf("Press Enter to stop...\n");
    getchar();
    std::printf("Terminating...\n");
    should_stop = true;

    uac_thread.join();

    return 0;
}