// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Common {

struct CrashInformation {
    std::vector<std::string> stack_trace;
    std::optional<std::string> minidump_filename;
};

void CrashHandler(std::function<void()> try_, std::function<void(const CrashInformation&)> catch_,
                  std::optional<std::string> minidump_filename = {});

} // namespace Common
