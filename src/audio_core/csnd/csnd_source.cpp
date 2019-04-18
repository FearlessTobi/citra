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
    channel_volume_left = command.channel_volume_left;
    channel_volume_right = command.channel_volume_right;
    capture_volume = command.capture_volume;
    first_block_phys_addr = command.first_block_phys_addr;
    second_block_phys_addr = command.second_block_phys_addr;
    total_size_of_one_block = command.total_size_of_one_block;
    sample_rate = 0x3FEC3FC / timer;
    LOG_CRITICAL(Frontend, "{}", sample_rate);

    u8* first_block_ptr =
        Core::System::GetInstance().Memory().GetPhysicalPointer(first_block_phys_addr);
    StereoBuffer16 first_buf = DecodeMemoryBlock(first_block_ptr);
    first_samples.resize(total_size_of_one_block * 2);
    for (int i = 0; i < first_buf.size(); i++) {
        for (int j = 0; j < first_buf[i].size(); j++) {
            first_samples[i + j] = first_buf[i][j];
        }
    }
    // Perform resampling if necessary
    Resample(first_samples);

    if (repeat_mode == 1) {
        u8* second_block_ptr =
            Core::System::GetInstance().Memory().GetPhysicalPointer(second_block_phys_addr);
        StereoBuffer16 second_buf = DecodeMemoryBlock(second_block_ptr);
        second_samples.resize(total_size_of_one_block * 2);
        for (int i = 0; i < second_buf.size(); i++) {
            for (int j = 0; j < second_buf[i].size(); j++) {
                second_samples[i + j] = second_buf[i][j];
            }
        }
        // Perform resampling if necessary
        Resample(second_samples);
    }
}

StereoBuffer16 CSNDSource::DecodeMemoryBlock(u8* ptr) {
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
    return buf;
}

void CSNDSource::Resample(std::vector<s16>& samples) {
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

void CSNDSource::EnablePlayback(bool enable) {
    enable_playback = enable;
}

StereoFrame16 CSNDSource::GenerateFrame() {
    StereoFrame16 output_frame;
    output_frame.fill({});

    // TODO: This skips stuff
    if (end_counter > total_size_of_one_block) {
        if (repeat_mode == 1) {
            play_second_block = true;
            offset = 0;
            end_counter = 0;
        } else {
            return output_frame;
        }
    }

    std::size_t size = output_frame.size();
    offset += size;
    end_counter += size * (double)((double)sample_rate / native_sample_rate) * mul_factor;

    if (play_second_block) {
        for (int i = 0; i < output_frame.size(); i++) {
            for (int j = 0; j < output_frame[i].size(); j++) {
                output_frame[i][j] = second_samples[i + j + offset];
            }
        }
    } else {
        for (int i = 0; i < output_frame.size(); i++) {
            for (int j = 0; j < output_frame[i].size(); j++) {
                output_frame[i][j] = first_samples[i + j + offset];
            }
        }
    }

    return output_frame;
}

} // namespace AudioCore::CSND
