// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstddef>
#include "audio_core/csnd/csnd_hle.h"
#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "common/assert.h"
#include "core/settings.h"

namespace AudioCore::CSND {

static constexpr u64 audio_frame_ticks = 1310252ull; ///< Units: ARM11 cycles

CsndHle::CsndHle() {
    Core::Timing& timing = Core::System::GetInstance().CoreTiming();
    tick_event =
        timing.RegisterEvent("AudioCore::CSND:::CsndHle::tick_event", [this](u64, s64 cycles_late) {
            this->AudioTickCallback(cycles_late);
        });
    timing.ScheduleEvent(audio_frame_ticks, tick_event);
}

CsndHle::~CsndHle() {
    Core::Timing& timing = Core::System::GetInstance().CoreTiming();
    timing.UnscheduleEvent(tick_event, 0);
}

static s16 ClampToS16(s32 value) {
    return static_cast<s16>(std::clamp(value, -32768, 32767));
}

static std::array<s16, 2> AddAndClampToS16(const std::array<s16, 2>& a,
                                           const std::array<s16, 2>& b) {
    return {ClampToS16(static_cast<s32>(a[0]) + static_cast<s32>(b[0])),
            ClampToS16(static_cast<s32>(a[1]) + static_cast<s32>(b[1]))};
}

StereoFrame16 CsndHle::GenerateCurrentFrame() {
    StereoFrame16 final_mix;
    final_mix.fill({});

    for (auto& source : sources) {
        MixInto(final_mix, source.Tick());
    }

    StereoFrame16 output_frame;
    output_frame.fill({});

    for (int i = 0; i < output_frame.size(); i++) {
        s16 left = ClampToS16(static_cast<s32>(final_mix[i][0]));
        s16 right = ClampToS16(static_cast<s32>(final_mix[i][1]));

        output_frame[i] = AddAndClampToS16(output_frame[i], {left, right});
    }

    return output_frame;
}

void CsndHle::MixInto(StereoFrame16& dest, StereoFrame16 source) {
    for (int i = 0; i < source.size(); i++) {
        for (int j = 0; j < source[i].size(); j++) {
            dest[i][j] += static_cast<s32>(source[i][j]);
        }
    }
}

bool CsndHle::Tick() {
    StereoFrame16 current_frame = {};

    current_frame = GenerateCurrentFrame();

    OutputFrame(current_frame);

    return true;
}

void CsndHle::AudioTickCallback(s64 cycles_late) {
    Tick();

    // Reschedule recurrent event
    Core::Timing& timing = Core::System::GetInstance().CoreTiming();
    timing.ScheduleEvent(audio_frame_ticks - cycles_late, tick_event);
}

void CsndHle::SetSink(const std::string& sink_id, const std::string& audio_device) {
    sink = CreateSinkFromID(Settings::values.sink_id, Settings::values.audio_device_id);
    sink->SetCallback(
        [this](s16* buffer, std::size_t num_frames) { OutputCallback(buffer, num_frames); });
    time_stretcher.SetOutputSampleRate(sink->GetNativeSampleRate());
}

Sink& CsndHle::GetSink() {
    ASSERT(sink);
    return *sink.get();
}

void CsndHle::EnableStretching(bool enable) {
    if (perform_time_stretching == enable)
        return;

    if (!enable) {
        flushing_time_stretcher = true;
    }
    perform_time_stretching = enable;
}

void CsndHle::OutputFrame(StereoFrame16& frame) {
    if (!sink)
        return;

    fifo.Push(frame.data(), frame.size());
}

void CsndHle::OutputSample(std::array<s16, 2> sample) {
    if (!sink)
        return;

    fifo.Push(&sample, 1);
}

void CsndHle::OutputCallback(s16* buffer, std::size_t num_frames) {
    std::size_t frames_written;
    if (perform_time_stretching) {
        const std::vector<s16> in{fifo.Pop()};
        const std::size_t num_in{in.size() / 2};
        frames_written = time_stretcher.Process(in.data(), num_in, buffer, num_frames);
    } else if (flushing_time_stretcher) {
        time_stretcher.Flush();
        frames_written = time_stretcher.Process(nullptr, 0, buffer, num_frames);
        frames_written += fifo.Pop(buffer, num_frames - frames_written);
        flushing_time_stretcher = false;
    } else {
        frames_written = fifo.Pop(buffer, num_frames);
    }

    if (frames_written > 0) {
        std::memcpy(&last_frame[0], buffer + 2 * (frames_written - 1), 2 * sizeof(s16));
    }

    // Hold last emitted frame; this prevents popping.
    for (std::size_t i = frames_written; i < num_frames; i++) {
        std::memcpy(buffer + 2 * i, &last_frame[0], 2 * sizeof(s16));
    }

    // Implementation of the hardware volume slider with a dynamic range of 60 dB
    const float linear_volume = std::clamp(Settings::values.volume, 0.0f, 1.0f);
    if (linear_volume != 1.0) {
        const float volume_scale_factor =
            linear_volume == 0 ? 0 : std::exp(6.90775f * linear_volume) * 0.001f;
        for (std::size_t i = 0; i < num_frames; i++) {
            buffer[i * 2 + 0] = static_cast<s16>(buffer[i * 2 + 0] * volume_scale_factor);
            buffer[i * 2 + 1] = static_cast<s16>(buffer[i * 2 + 1] * volume_scale_factor);
        }
    }
}

} // namespace AudioCore::CSND
