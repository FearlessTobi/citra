// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <SDL.h>
#include "audio_core/csnd/csnd_source.h"
#include "core/core.h"

namespace AudioCore::CSND {

// TODO: Respect channel volume, repeat mode, encoding
// TODO: Fix crashes when buf size is too small
// TODO: Check what happens with DSP and CSND at once (applets)

CSNDSource::CSNDSource(Service::CSND::Command0xE command) {
    channel_index = command.flags_timer.channel_index;
    enable_linear_interpolation = command.flags_timer.enable_linear_interpolation;
    repeat_mode = command.flags_timer.repeat_mode;
    encoding = command.flags_timer.encoding;
    enable_playback = command.flags_timer.enable_playback;
    timer = command.flags_timer.timer;
    channel_volume = command.channel_volume;
    capture_volume = command.capture_volume;
    first_block_phys_addr = command.first_block_phys_addr;
    second_block_phys_addr = command.second_block_phys_addr;
    total_size_of_one_block = command.total_size_of_one_block;
    sample_rate = 0x3FEC3FC / timer;
    LOG_CRITICAL(Frontend, "{}", sample_rate);

    // TODO: Support other options
    ptr = Core::System::GetInstance().Memory().GetPhysicalPointer(first_block_phys_addr);

    StereoBuffer16 buf(total_size_of_one_block);
    switch (encoding) {
    case 0:
        buf = Codec::DecodePCM8(1, ptr, total_size_of_one_block);
        mul_factor = 1;
        break;
    case 1:
        buf = Codec::DecodePCM16(1, ptr, total_size_of_one_block);
        mul_factor = 2;
        break;
    case 2:
        LOG_CRITICAL(Frontend, "IMA-ADPCM is unimplemented!");
        mul_factor = 4;
        break;
    case 3:
        LOG_CRITICAL(Frontend, "PSG is unimplemented!");
        mul_factor = 2;
        break;
    default:
        LOG_CRITICAL(Frontend, "Encoding {} is unimplemented!", encoding);
        mul_factor = 1;
        break;
    }

    samples.resize(total_size_of_one_block * 2);
    for (int i = 0; i < buf.size(); i++) {
        for (int j = 0; j < buf[i].size(); j++) {
            samples[i + j] = buf[i][j];
        }
    }

    // Perform resampling if necessary
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, AUDIO_S16, 2, sample_rate, AUDIO_S16, 2, native_sample_rate);
    if (cvt.needed) {
        cvt.len = samples.size() * 2; // an s16 sample is 2 bytes long
        cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);

        std::memcpy(cvt.buf, samples.data(), cvt.len);
        SDL_ConvertAudio(&cvt);

        samples.resize(cvt.len_cvt);
        std::memcpy(samples.data(), cvt.buf, cvt.len_cvt);
    }
}

CSNDSource::~CSNDSource() {}

StereoFrame16 CSNDSource::Tick() {
    StereoFrame16 frame;
    frame.fill({});

    if (enable_playback) {
        frame = GenerateFrame();
    }

    return frame;
}

u32 CSNDSource::GetChannelIndex() {
    return channel_index;
}

StereoFrame16 CSNDSource::GenerateFrame() {
    StereoFrame16 output_frame;
    output_frame.fill({});

    // TODO: This skips stuff
    if (end_counter > total_size_of_one_block) {
        return output_frame;
    }

    std::size_t size = output_frame.size();
    offset += size;
    end_counter += size * (double)((double)sample_rate / native_sample_rate) * mul_factor;
    for (int i = 0; i < output_frame.size(); i++) {
        for (int j = 0; j < output_frame[i].size(); j++) {
            output_frame[i][j] = samples[i + j + offset];
        }
    }

    return output_frame;
}

} // namespace AudioCore::CSND
