#include "Trace.h"

#include "../../src/Diagnostics/TraceSink.h"

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>

#include <Windows.h>

#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace wac::trace {
namespace {
class TraceState final {
public:
    ~TraceState() {
        if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_);
    }

    std::mutex mutex;
    HANDLE file_{INVALID_HANDLE_VALUE};
    uint64_t sequence{0};
    bool initialized{false};
};

TraceState& State() noexcept {
    static TraceState state;
    return state;
}

std::wstring PackageFamily() noexcept {
    try {
        return winrt::Windows::ApplicationModel::Package::Current().Id().FamilyName().c_str();
    } catch (...) {
        return L"unknown";
    }
}

std::string Utf8(std::wstring_view value) {
    if (value.empty()) return {};
    const auto length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (length <= 0) return {};
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length,
                        nullptr, nullptr);
    return result;
}

void Persist(std::wstring const& line) noexcept {
    auto& state = State();
    OutputDebugStringW(line.c_str());
    if (state.file_ == INVALID_HANDLE_VALUE) return;

    try {
        const auto bytes = Utf8(line);
        if (bytes.empty()) return;
        DWORD written = 0;
        if (WriteFile(state.file_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)) {
            FlushFileBuffers(state.file_);
        }
    } catch (...) {
    }
}

void CoreSink(std::wstring_view area, std::wstring_view message) noexcept { Write(area, message); }
}

void Initialize() noexcept {
    bool first_initialization = false;
    {
        auto& state = State();
        std::lock_guard lock(state.mutex);
        if (state.initialized) return;
        state.initialized = true;
        first_initialization = true;

        try {
            const auto local_folder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();
            const auto path = std::filesystem::path{local_folder.c_str()} / L"wac-trace.log";
            state.file_ = CreateFileW(path.c_str(), FILE_APPEND_DATA,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        } catch (...) {
            state.file_ = INVALID_HANDLE_VALUE;
        }
    }

    if (!first_initialization) return;
    wac::trace_sink::SetSink(&CoreSink);
    Write(L"TRACE", L"session pid=" + std::to_wstring(GetCurrentProcessId()) +
                         L" package=" + PackageFamily());
}

void Write(std::wstring_view area, std::wstring_view message) noexcept {
    try {
        auto& state = State();
        std::lock_guard lock(state.mutex);
        SYSTEMTIME now{};
        GetLocalTime(&now);
        std::wostringstream line;
        line << std::setfill(L'0') << std::setw(6) << ++state.sequence << L' '
             << std::setw(2) << now.wHour << L':' << std::setw(2) << now.wMinute << L':'
             << std::setw(2) << now.wSecond << L'.' << std::setw(3) << now.wMilliseconds
             << L" T" << GetCurrentThreadId() << L' ' << std::left << std::setw(8) << area
             << L' ' << message << L'\n';
        Persist(line.str());
    } catch (...) {
        try {
            OutputDebugStringW(L"WAC TRACE FAILURE\n");
        } catch (...) {
        }
    }
}

void WriteHr(std::wstring_view area, std::wstring_view operation, HRESULT result) noexcept {
    try {
        std::wostringstream message;
        message << operation << L" hr=0x" << std::uppercase << std::hex << std::setfill(L'0')
                << std::setw(8) << static_cast<unsigned long>(result);
        Write(area, message.str());
    } catch (...) {
    }
}

uint64_t NextOperationId() noexcept { return wac::trace_sink::NextOperationId(); }
}
