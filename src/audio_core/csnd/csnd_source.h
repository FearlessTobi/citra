// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "audio_core/audio_types.h"
#include "audio_core/codec.h"
#include "core/hle/service/csnd/csnd_snd.h"

namespace Service::CSND {
struct Command0xE;
}

namespace AudioCore::CSND {

class CSNDSource {

public:
    explicit CSNDSource(Service::CSND::Command0xE command);
    ~CSNDSource();

    StereoFrame16 Tick();

    u32 GetChannelIndex();

private:
    StereoFrame16 GenerateFrame();

    u32 channel_index = 0;
    u32 enable_linear_interpolation = 0;
    u32 repeat_mode = 0;
    u32 encoding = 0;
    u32 enable_playback = 0;
    u32 timer = 0;
    u32 channel_volume = 0;
    u32 capture_volume = 0;
    u32 first_block_phys_addr = 0;
    u32 second_block_phys_addr = 0;
    u32 total_size_of_one_block = 0;
    u32 sample_rate = 0;
    // TODO: Still not really a good place
    u32 mul_factor = 0;

    u8* ptr;

    std::size_t offset = 0;
    double end_counter = 0;

    std::vector<s16> samples;
};

} // namespace AudioCore::CSND
