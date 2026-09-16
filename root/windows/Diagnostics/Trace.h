#pragma once

#include <windows.h>

#include <cstdint>
#include <string_view>

namespace wac::trace {
void Initialize() noexcept;

void Write(std::wstring_view area, std::wstring_view message) noexcept;

void WriteHr(std::wstring_view area, std::wstring_view operation, HRESULT result) noexcept;

uint64_t NextOperationId() noexcept;
}
