// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "audio_core/audio_types.h"
#include "audio_core/csnd/csnd_source.h"
#include "audio_core/sink.h"
#include "audio_core/time_stretch.h"
#include "common/common_types.h"
#include "common/ring_buffer.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/memory.h"

namespace AudioCore::CSND {

class CSNDSource;

class CsndHle final {
public:
    explicit CsndHle();
    ~CsndHle();

    /// Select the sink to use based on sink id.
    void SetSink(const std::string& sink_id, const std::string& audio_device);
    /// Enable/Disable audio stretching.
    void EnableStretching(bool enable);

    std::vector<CSNDSource> sources;

private:
    StereoFrame16 GenerateCurrentFrame();
    void MixInto(StereoFrame16& dest, StereoFrame16 source);
    bool Tick();
    void AudioTickCallback(s64 cycles_late);

    /// Get the current sink
    Sink& GetSink();

    void OutputFrame(StereoFrame16& frame);
    void OutputSample(std::array<s16, 2> sample);

    void OutputCallback(s16* buffer, std::size_t num_frames);

    Core::TimingEventType* tick_event;

    std::unique_ptr<Sink> sink;
    std::atomic<bool> perform_time_stretching = false;
    std::atomic<bool> flushing_time_stretcher = false;
    Common::RingBuffer<s16, 0x2000, 2> fifo;
    std::array<s16, 2> last_frame{};
    TimeStretcher time_stretcher;
};

} // namespace AudioCore::CSND
