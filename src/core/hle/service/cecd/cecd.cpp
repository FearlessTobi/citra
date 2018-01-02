// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/result.h"
#include "core/hle/service/cecd/cecd.h"
#include "core/hle/service/cecd/cecd_ndm.h"
#include "core/hle/service/cecd/cecd_s.h"
#include "core/hle/service/cecd/cecd_u.h"
#include "core/hle/service/service.h"


namespace Service {
namespace CECD {

enum class SaveDataType {
    Invalid = 0,
    MBoxList = 1,
    MBoxInfo = 2,
    InBoxInfo = 3,
    OutBoxInfo = 4,
    OutBoxIndex = 5,
    InBoxMessage = 6,
    OutBoxMessage = 7,
    RootDir = 10,
    MBoxDir = 11,
    InBoxDir = 12,
    OutBoxDir = 13,
    MBoxDataStart = 100,
    MBoxDataProgramId = 150,
    MBoxDataEnd = 199
};

union FileOption {
    u32 raw;
    BitField<1, 1, u32> read;
    BitField<2, 1, u32> write;
    BitField<3, 1, u32> make_dir;
    BitField<4, 1, u32> no_check;
    BitField<30, 1, u32> dump;
};

static Kernel::SharedPtr<Kernel::Event> cecinfo_event;
static Kernel::SharedPtr<Kernel::Event> change_state_event;

void GetCecStateAbbreviated(Service::Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    cmd_buff[2] = static_cast<u32>(CecStateAbbreviated::CEC_STATE_ABBREV_IDLE);

    LOG_WARNING(Service_CECD, "(STUBBED) called");
}

void GetCecInfoEventHandle(Service::Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    cmd_buff[3] = Kernel::g_handle_table.Create(cecinfo_event).Unwrap(); // Event handle

    LOG_WARNING(Service_CECD, "(STUBBED) called");
}

void GetChangeStateEventHandle(Service::Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    cmd_buff[3] = Kernel::g_handle_table.Create(change_state_event).Unwrap(); // Event handle

    LOG_WARNING(Service_CECD, "(STUBBED) called");
}

void OpenAndRead(Service::Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x12, 4, 4);

    u32 size = rp.Pop<u32>();
    u32 title_id = rp.Pop<u32>();
    auto save_data_type = static_cast<SaveDataType>(rp.Pop<u32>());
    FileOption option{rp.Pop<u32>()};
    rp.Skip(2, false); // PID
    size_t buffer_size;
    IPC::MappedBufferPermissions perm;
    VAddr buffer_address = rp.PopMappedBuffer(&buffer_size, &perm);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 2);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0);
    rb.PushMappedBuffer(buffer_address, buffer_size, perm);

    LOG_WARNING(Service_CECD, "(STUBBED) called. title_id = 0x%08X, save_data_type = %d, option = "
        "0x%08X, buffer_address = 0x%08X, size = 0x%X",
        title_id, save_data_type, option, buffer_address, size);
}


void Init() {
    AddService(new CECD_NDM);
    AddService(new CECD_S);
    AddService(new CECD_U);

    cecinfo_event = Kernel::Event::Create(Kernel::ResetType::OneShot, "CECD::cecinfo_event");
    change_state_event =
        Kernel::Event::Create(Kernel::ResetType::OneShot, "CECD::change_state_event");
}

void Shutdown() {
    cecinfo_event = nullptr;
    change_state_event = nullptr;
}

} // namespace CECD

} // namespace Service
