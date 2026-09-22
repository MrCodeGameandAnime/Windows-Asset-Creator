#include "TraceSink.h"

#include <atomic>
#include <iomanip>
#include <sstream>

namespace wac::trace_sink {
namespace {
std::atomic<Sink> sink{nullptr};
std::atomic_bool diagnostics_enabled{false};
std::atomic_uint64_t operation_ids{0};
thread_local std::wstring operation_tag;
}

void SetDiagnosticsEnabled(bool enabled) noexcept {
    diagnostics_enabled.store(enabled, std::memory_order_release);
}

bool DiagnosticsEnabled() noexcept {
    return diagnostics_enabled.load(std::memory_order_acquire);
}

void SetSink(Sink value) noexcept { sink.store(value, std::memory_order_release); }

void Emit(std::wstring_view area, std::wstring_view message) noexcept {
    if (!DiagnosticsEnabled()) return;
    const auto callback = sink.load(std::memory_order_acquire);
    if (!callback) return;
    try {
        if (operation_tag.empty()) {
            callback(area, message);
            return;
        }

        std::wstring tagged = operation_tag;
        tagged.append(L" ");
        tagged.append(message);
        callback(area, tagged);
    } catch (...) {
    }
}

void EmitHr(std::wstring_view area, std::wstring_view operation, long result) noexcept {
    if (!DiagnosticsEnabled()) return;
    try {
        std::wostringstream message;
        message << operation << L" hr=0x" << std::uppercase << std::hex << std::setfill(L'0')
                << std::setw(8) << static_cast<unsigned long>(result);
        Emit(area, message.str());
    } catch (...) {
    }
}

uint64_t NextOperationId() noexcept { return operation_ids.fetch_add(1, std::memory_order_relaxed) + 1; }

void SetOperationTag(std::wstring_view tag) noexcept {
    try {
        operation_tag.assign(tag);
    } catch (...) {
        operation_tag.clear();
    }
}

void ClearOperationTag() noexcept { operation_tag.clear(); }

bool HasOperationTag() noexcept { return !operation_tag.empty(); }

OperationTagScope::OperationTagScope(std::wstring_view tag) noexcept {
    try {
        previous_ = operation_tag;
        operation_tag.assign(tag);
        active_ = true;
    } catch (...) {
    }
}

OperationTagScope::~OperationTagScope() noexcept {
    if (!active_) return;
    try {
        operation_tag = std::move(previous_);
    } catch (...) {
        operation_tag.clear();
    }
}
}
