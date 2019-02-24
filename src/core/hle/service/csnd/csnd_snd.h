// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::CSND {

struct Type0Command {
    // command id and next command offset
    u16 offset;
    u16 command_id;
    u32 finished;
    u32 flags;
    u8 parameters[20];
};
static_assert(sizeof(Type0Command) == 0x20, "Type0Command structure size is wrong");

struct Command0x1 {
    // command id and next command offset
    u16 offset;
    u16 command_id;
    u32 play; // stop=0, play=1
    INSERT_PADDING_BYTES(0x10);
};
static_assert(sizeof(Type0Command) == 0x20, "Type0Command structure size is wrong");

#pragma pack(push, 1)
struct Command0xE {
    // command id and next command offset
    // comment is position after!!
    u16 offset;              // 0x2
    u16 command_id;          // 0x4
    INSERT_PADDING_BYTES(4); // 0x8
    union {
        u32 raw;

        BitField<0, 5, u32> channel_index;
        BitField<5, 1, u32> enable_linear_interpolation;
        BitField<6, 3, u32> ignored;
        BitField<9, 2, u32> repeat_mode;
        BitField<11, 2, u32> encoding;
        BitField<13, 1, u32> enable_playback;
        BitField<14, 1, u32> ignored2;
        BitField<15, 17, u32> sample_rate;
    } flags_timer;
    u32 channel_volume;          // 0x10
    u32 capture_volume;          // 0x14
    u32 first_block_phys_addr;   // 0x18
    u32 second_block_phys_addr;  // 0x1C
    u32 total_size_of_one_block; // 0x20
};
static_assert(sizeof(Command0xE) == 0x20, "Command0xE structure size is wrong");
#pragma pack(pop)

class CSND_SND final : public ServiceFramework<CSND_SND> {

public:
    explicit CSND_SND(Core::System& system);
    ~CSND_SND() = default;

private:
    /**
     * CSND_SND::Initialize service function
     *  Inputs:
     *      0 : Header Code[0x00010140]
     *      1 : Shared memory block size, for mem-block creation
     *      2 : Offset0 located in the shared-memory, region size=8
     *      3 : Offset1 located in the shared-memory, region size=12*num_channels
     *      4 : Offset2 located in the shared-memory, region size=8*num_capturedevices
     *      5 : Offset3 located in the shared-memory.
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     *      2 : Handle-list header
     *      3 : Mutex handle
     *      4 : Shared memory block handle
     */
    void Initialize(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::Shutdown service function
     *  Inputs:
     *      0 : Header Code[0x00020000]
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Shutdown(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::ExecuteCommands service function
     *  Inputs:
     *      0 : Header Code[0x00030040]
     *      1 : Command offset in shared memory.
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void ExecuteCommands(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::ExecuteType1Commands service function
     *  Inputs:
     *      0 : Header Code[0x00040080]
     *      1 : unknown
     *      2 : unknown
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void ExecuteType1Commands(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::AcquireSoundChannels service function
     *  Inputs:
     *      0 : Header Code[0x00050000]
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     *      2 : Available channel bit mask
     */
    void AcquireSoundChannels(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::ReleaseSoundChannels service function
     *  Inputs:
     *      0 : Header Code[0x00060000]
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void ReleaseSoundChannels(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::AcquireCapUnit service function
     *     This function tries to acquire one capture device (max: 2).
     *     Returns index of which capture device was acquired.
     *  Inputs:
     *      0 : Header Code[0x00070000]
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     *      2 : Capture Unit
     */
    void AcquireCapUnit(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::ReleaseCapUnit service function
     *  Inputs:
     *      0 : Header Code[0x00080040]
     *      1 : Capture Unit
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void ReleaseCapUnit(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::FlushDataCache service function
     *
     * This Function is a no-op, We aren't emulating the CPU cache any time soon.
     *
     *  Inputs:
     *      0 : Header Code[0x00090082]
     *      1 : Address
     *      2 : Size
     *      3 : Value 0, some descriptor for the KProcess Handle
     *      4 : KProcess handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void FlushDataCache(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::StoreDataCache service function
     *
     * This Function is a no-op, We aren't emulating the CPU cache any time soon.
     *
     *  Inputs:
     *      0 : Header Code[0x000A0082]
     *      1 : Address
     *      2 : Size
     *      3 : Value 0, some descriptor for the KProcess Handle
     *      4 : KProcess handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void StoreDataCache(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::InvalidateDataCache service function
     *
     * This Function is a no-op, We aren't emulating the CPU cache any time soon.
     *
     *  Inputs:
     *      0 : Header Code[0x000B0082]
     *      1 : Address
     *      2 : Size
     *      3 : Value 0, some descriptor for the KProcess Handle
     *      4 : KProcess handle
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void InvalidateDataCache(Kernel::HLERequestContext& ctx);

    /**
     * CSND_SND::Reset service function
     *  Inputs:
     *      0 : Header Code[0x000C0000]
     *  Outputs:
     *      1 : Result of function, 0 on success, otherwise error code
     */
    void Reset(Kernel::HLERequestContext& ctx);

    Core::System& system;

    Kernel::SharedPtr<Kernel::Mutex> mutex = nullptr;
    Kernel::SharedPtr<Kernel::SharedMemory> shared_memory = nullptr;

    static constexpr u32 MaxCaptureUnits = 2;
    std::array<bool, MaxCaptureUnits> capture_units = {false, false};
};

/// Initializes the CSND_SND Service
void InstallInterfaces(Core::System& system);

using StereoBuffer16 = std::deque<std::array<s16, 2>>;

StereoBuffer16 DecodePCM8(const std::vector<u8> data, const std::size_t sample_count,
                          const std::size_t offset);

std::vector<int> stop_threads;
std::map<int, std::thread> slot_threads;
} // namespace Service::CSND
