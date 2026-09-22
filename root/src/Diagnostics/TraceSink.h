#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace wac::trace_sink {
using Sink = void (*)(std::wstring_view area, std::wstring_view message) noexcept;

void SetDiagnosticsEnabled(bool enabled) noexcept;
bool DiagnosticsEnabled() noexcept;
void SetSink(Sink sink) noexcept;
void Emit(std::wstring_view area, std::wstring_view message) noexcept;
void EmitHr(std::wstring_view area, std::wstring_view operation, long result) noexcept;
uint64_t NextOperationId() noexcept;
void SetOperationTag(std::wstring_view tag) noexcept;
void ClearOperationTag() noexcept;
bool HasOperationTag() noexcept;

class OperationTagScope final {
public:
    explicit OperationTagScope(std::wstring_view tag) noexcept;
    OperationTagScope(OperationTagScope const&) = delete;
    OperationTagScope& operator=(OperationTagScope const&) = delete;
    ~OperationTagScope() noexcept;

private:
    std::wstring previous_;
    bool active_{false};
};
}
