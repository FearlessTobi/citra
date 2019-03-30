// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <math.h>
#include "audio_core/dsp_interface.h"
#include "common/alignment.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/result.h"
#include "core/hle/service/csnd/csnd_snd.h"

namespace Service::CSND {

/// Creates a CSND timer value from a sample rate.
#define CSND_TIMER(n) (0x3FEC3FC / ((u32)(n)))

void CSND_SND::Initialize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x01, 5, 0);
    const u32 size = Common::AlignUp(rp.Pop<u32>(), Memory::PAGE_SIZE);
    const u32 offset0 = rp.Pop<u32>();
    const u32 offset1 = rp.Pop<u32>();
    const u32 offset2 = rp.Pop<u32>();
    const u32 offset3 = rp.Pop<u32>();

    using Kernel::MemoryPermission;
    mutex = system.Kernel().CreateMutex(false, "CSND:mutex");
    shared_memory = system.Kernel()
                        .CreateSharedMemory(nullptr, size, MemoryPermission::ReadWrite,
                                            MemoryPermission::ReadWrite, 0,
                                            Kernel::MemoryRegion::BASE, "CSND:SharedMemory")
                        .Unwrap();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 3);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(mutex, shared_memory);

    LOG_WARNING(Service_CSND,
                "(STUBBED) called, size=0x{:08X} "
                "offset0=0x{:08X} offset1=0x{:08X} offset2=0x{:08X} offset3=0x{:08X}",
                size, offset0, offset1, offset2, offset3);
}

void CSND_SND::Shutdown(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x02, 0, 0);

    if (mutex)
        mutex = nullptr;
    if (shared_memory)
        shared_memory = nullptr;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_CRITICAL(Service_CSND, "(STUBBED) called");
}

void CSND_SND::Timer(u64 userdata, s64 cycles_late) {
    auto& timing = Core::System::GetInstance().CoreTiming();

    if (!(count > buffer.size())) {
        Core::System::GetInstance().DSP().OutputStereoBuf(buffer, 1, count);
        Core::System::GetInstance().DSP().OutputStereoBuf(buffer, 1, count);
        Core::System::GetInstance().DSP().OutputStereoBuf(buffer, 1, count);
        count++;
    } else {
        LOG_CRITICAL(Service_CSND, "Second {} {}", count, buffer.size());
    }

    // schedule next run
    timing.ScheduleEvent(sample_rate_timer - cycles_late, timer_event);
}

void CSND_SND::Process0xE(Type0Command command, u8* ptr) {
    Command0xE command_0xE;
    std::memcpy(&command_0xE, ptr, sizeof(Command0xE));

    std::vector<u8> data;
    data.resize(command_0xE.total_size_of_one_block);
    int size = data.size();
    printf("%i\n", size);

    u8* POINTER =
        Core::System::GetInstance().Memory().GetPhysicalPointer(command_0xE.first_block_phys_addr);

    int sr = command_0xE.flags_timer.sample_rate;
    sample_rate_timer = CSND_TIMER(sr);
    auto& timing = Core::System::GetInstance().CoreTiming();

    std::memcpy(&data[0], POINTER, size);
    int sqrt = std::sqrt(sr);
    // for xump ~110
    /*for (int i = 0; i < data.size(); i += sqrt) { // TODO: This loop doesn't cover everything
        StereoBuffer16 buf = DecodePCM8(data, sqrt, i);
        Core::System::GetInstance().DSP().OutputStereoBuf(buf);
        std::this_thread::sleep_for(std::chrono::microseconds(1000000 / sqrt));
    }*/

    buffer = DecodePCM8(data, data.size(), 0);
    count = 0;
    if (!started) {
        LOG_WARNING(Frontend, "Starting!");
        timer_event = timing.RegisterEvent(
            "CSND::Timer", [this](u64 userdata, s64 cycles_late) { Timer(userdata, cycles_late); });
        timing.ScheduleEvent(sample_rate_timer, timer_event);
        started = true;
    }
}

void CSND_SND::ProcessCommand(Type0Command command, u8* ptr) {
    Command0x1 command_0x1;
    std::memcpy(&command_0x1, ptr, sizeof(Command0x1));
    if (command.command_id == 0x1) {
        LOG_CRITICAL(Service_CSND, "PLAY: {}", command_0x1.play);
    }

    if (command.command_id == 0xE) {
        // TODO: Channel Index
        Command0xE command_0xE;
        std::memcpy(&command_0xE, ptr, sizeof(Command0xE));
        LOG_CRITICAL(Service_CSND, "offset: 0x{:08X}", command_0xE.offset);
        LOG_CRITICAL(Service_CSND, "flags_timer: 0x{:08X}", command_0xE.flags_timer.raw);
        LOG_CRITICAL(Service_CSND, "flags_timer.channel_index: 0x{:08X}",
                     command_0xE.flags_timer.channel_index);
        LOG_CRITICAL(Service_CSND, "flags_timer.enable_linear_interpolation: 0x{:08X}",
                     command_0xE.flags_timer.enable_linear_interpolation);
        LOG_CRITICAL(Service_CSND, "flags_timer.repeat_mode: 0x{:08X}",
                     command_0xE.flags_timer.repeat_mode);
        LOG_CRITICAL(Service_CSND, "flags_timer.encoding: 0x{:08X}",
                     command_0xE.flags_timer.encoding);
        LOG_CRITICAL(Service_CSND, "flags_timer.enable_playback: 0x{:08X}",
                     command_0xE.flags_timer.enable_playback);
        LOG_CRITICAL(Service_CSND, "flags_timer.sample_rate: 0x{:08X}",
                     command_0xE.flags_timer.sample_rate);
        LOG_CRITICAL(Service_CSND, "channel_volume: 0x{:08X}", command_0xE.channel_volume);
        LOG_CRITICAL(Service_CSND, "capture_volume: 0x{:08X}", command_0xE.capture_volume);
        LOG_CRITICAL(Service_CSND, "first_block_phys_addr: 0x{:08X}",
                     command_0xE.first_block_phys_addr);
        LOG_CRITICAL(Service_CSND, "second_block_phys_addr: 0x{:08X}",
                     command_0xE.second_block_phys_addr);
        LOG_CRITICAL(Service_CSND, "total_size_of_one_block: 0x{:08X}",
                     command_0xE.total_size_of_one_block);

        int i = command_0xE.flags_timer.channel_index;

        Process0xE(command, ptr);
    }
}

void CSND_SND::ExecuteCommands(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x03, 1, 0);
    const u32 addr = rp.Pop<u32>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    if (!shared_memory) {
        rb.Push<u32>(1);
        LOG_ERROR(Service_CSND, "called, shared memory not allocated");
    } else {
        u8* ptr = shared_memory->GetPointer(addr);
        Type0Command command;

        std::memcpy(&command, ptr, sizeof(Type0Command));
        command.finished |= 1;
        std::memcpy(ptr, &command, sizeof(Type0Command));
        LOG_CRITICAL(Service_CSND, "command_id: 0x{:04X}", command.command_id);

        ProcessCommand(command, ptr);

        rb.Push(RESULT_SUCCESS);
    }

    LOG_CRITICAL(Service_CSND, "(STUBBED) called, addr=0x{:08X}", addr);
}

void CSND_SND::AcquireSoundChannels(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x05, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0xFFFFFF00);

    LOG_CRITICAL(Service_CSND, "(STUBBED) called");
}

void CSND_SND::ReleaseSoundChannels(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x06, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_CRITICAL(Service_CSND, "(STUBBED) called");
}

void CSND_SND::AcquireCapUnit(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x7, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    if (capture_units[0] && capture_units[1]) {
        LOG_WARNING(Service_CSND, "No more capture units available");
        rb.Push(ResultCode(ErrorDescription::InvalidResultValue, ErrorModule::CSND,
                           ErrorSummary::OutOfResource, ErrorLevel::Status));
        rb.Skip(1, false);
        return;
    }
    rb.Push(RESULT_SUCCESS);

    if (capture_units[0]) {
        capture_units[1] = true;
        rb.Push<u32>(1);
    } else {
        capture_units[0] = true;
        rb.Push<u32>(0);
    }

    LOG_CRITICAL(Service_CSND, "(STUBBED) called");
}

void CSND_SND::ReleaseCapUnit(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x8, 1, 0);
    const u32 index = rp.Pop<u32>();

    capture_units[index] = false;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_CRITICAL(Service_CSND, "(STUBBED) called, capture_unit_index={}", index);
}

void CSND_SND::FlushDataCache(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x9, 2, 2);
    const VAddr address = rp.Pop<u32>();
    const u32 size = rp.Pop<u32>();
    const auto process = rp.PopObject<Kernel::Process>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_TRACE(Service_CSND, "(STUBBED) called address=0x{:08X}, size=0x{:08X}, process={}", address,
              size, process->process_id);
}

void CSND_SND::StoreDataCache(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0xA, 2, 2);
    const VAddr address = rp.Pop<u32>();
    const u32 size = rp.Pop<u32>();
    const auto process = rp.PopObject<Kernel::Process>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_TRACE(Service_CSND, "(STUBBED) called address=0x{:08X}, size=0x{:08X}, process={}", address,
              size, process->process_id);
}

void CSND_SND::InvalidateDataCache(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0xB, 2, 2);
    const VAddr address = rp.Pop<u32>();
    const u32 size = rp.Pop<u32>();
    const auto process = rp.PopObject<Kernel::Process>();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_TRACE(Service_CSND, "(STUBBED) called address=0x{:08X}, size=0x{:08X}, process={}", address,
              size, process->process_id);
}

void CSND_SND::Reset(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0xC, 0, 0);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_CRITICAL(Service_CSND, "(STUBBED) called");
}

CSND_SND::CSND_SND(Core::System& system) : ServiceFramework("csnd:SND", 4), system(system) {
    static const FunctionInfo functions[] = {
        // clang-format off
        {0x00010140, &CSND_SND::Initialize, "Initialize"},
        {0x00020000, &CSND_SND::Shutdown, "Shutdown"},
        {0x00030040, &CSND_SND::ExecuteCommands, "ExecuteCommands"},
        {0x00040080, nullptr, "ExecuteType1Commands"},
        {0x00050000, &CSND_SND::AcquireSoundChannels, "AcquireSoundChannels"},
        {0x00060000, &CSND_SND::ReleaseSoundChannels, "ReleaseSoundChannels"},
        {0x00070000, &CSND_SND::AcquireCapUnit, "AcquireCapUnit"},
        {0x00080040, &CSND_SND::ReleaseCapUnit, "ReleaseCapUnit"},
        {0x00090082, &CSND_SND::FlushDataCache, "FlushDataCache"},
        {0x000A0082, &CSND_SND::StoreDataCache, "StoreDataCache"},
        {0x000B0082, &CSND_SND::InvalidateDataCache, "InvalidateDataCache"},
        {0x000C0000, &CSND_SND::Reset, "Reset"},
        // clang-format on
    };

    RegisterHandlers(functions);
};

void InstallInterfaces(Core::System& system) {
    auto& service_manager = system.ServiceManager();
    std::make_shared<CSND_SND>(system)->InstallAsService(service_manager);
}

StereoBuffer16 DecodePCM8(const std::vector<u8> data, const std::size_t sample_count,
                          const std::size_t offset) {
    const auto decode_sample = [](u8 sample) {
        return static_cast<s16>(static_cast<u16>(sample) << 8);
    };

    StereoBuffer16 ret(sample_count);

    for (std::size_t i = 0; i < sample_count; i++) {
        ret[i].fill(decode_sample(data[i + offset]));
    }

    return ret;
}

} // namespace Service::CSND
