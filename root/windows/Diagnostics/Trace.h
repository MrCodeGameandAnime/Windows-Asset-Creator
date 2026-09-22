#pragma once

#include <windows.h>

#include <cstdint>
#include <string_view>

namespace wac::trace {
void EnableDiagnostics() noexcept;
bool DiagnosticsEnabled() noexcept;
void Initialize() noexcept;

#ifdef _DEBUG
// In development packages, asks once for a folder grant and sends the trace
// directly to wac-trace.log there. Release builds do not expose this path.
void ConfigureDevelopmentOutput(HWND owner) noexcept;
#endif

void Write(std::wstring_view area, std::wstring_view message) noexcept;

void WriteHr(std::wstring_view area, std::wstring_view operation, HRESULT result) noexcept;

uint64_t NextOperationId() noexcept;
}
