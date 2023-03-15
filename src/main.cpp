#include <stdio.h>
#include <stdlib.h>
//
#include <ios>
#include <iostream>
#include <atomic>
//
#include "audio_engine.hpp"
#include "sip_agent.hpp"

#include "sdp.hpp"


int main(int /*argc*/, char** /*argv*/)
{
    cliph::audio::engine::get().init();
    cliph::sip::agent::get().run();
    //
    std::printf("Press Enter to stop...\n");
    getchar();
    std::printf("Terminating...\n");
    //
    cliph::sip::agent::get().stop();
}