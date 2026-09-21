#include "Trace.h"

#include "../../src/Diagnostics/TraceSink.h"
#include "../../src/Diagnostics/TraceLineBuffer.h"

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#ifdef _DEBUG
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Streams.h>
#include <shobjidl_core.h>
#endif

#include <Windows.h>

#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

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
#ifdef _DEBUG
    winrt::Windows::Storage::StorageFile development_file{nullptr};
    TraceLineBuffer development_pending;
    bool development_writer_active{false};
#endif
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
#ifdef _DEBUG
    try {
        state.development_pending.Enqueue(line);
    } catch (...) {
    }
#endif
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

#ifdef _DEBUG
winrt::fire_and_forget DrainDevelopmentOutputAsync() {
    co_await winrt::resume_background();
    auto& state = State();
    try {
        while (true) {
            winrt::Windows::Storage::StorageFile destination{nullptr};
            std::vector<std::wstring> lines;
            {
                std::lock_guard lock(state.mutex);
                destination = state.development_file;
                lines = state.development_pending.TakeAll();
                if (!destination || lines.empty()) {
                    state.development_writer_active = false;
                    co_return;
                }
            }

            std::wstring batch;
            size_t length = 0;
            for (const auto& line : lines) length += line.size();
            batch.reserve(length);
            for (const auto& line : lines) batch.append(line);
            co_await winrt::Windows::Storage::FileIO::AppendTextAsync(
                destination, batch, winrt::Windows::Storage::Streams::UnicodeEncoding::Utf8);
        }
    } catch (winrt::hresult_error const& error) {
        {
            std::lock_guard lock(state.mutex);
            state.development_file = nullptr;
            state.development_writer_active = false;
        }
        WriteHr(L"TRACE", L"development log write failed", error.code());
        Write(L"TRACE", L"development log disabled; LocalState fallback remains active");
    } catch (...) {
        {
            std::lock_guard lock(state.mutex);
            state.development_file = nullptr;
            state.development_writer_active = false;
        }
        Write(L"TRACE", L"development log write failed; LocalState fallback remains active");
    }
}

void StartDevelopmentWriter() noexcept {
    auto& state = State();
    {
        std::lock_guard lock(state.mutex);
        if (!state.development_file || state.development_writer_active || state.development_pending.Empty()) return;
        state.development_writer_active = true;
    }
    try {
        DrainDevelopmentOutputAsync();
    } catch (...) {
        std::lock_guard lock(state.mutex);
        state.development_writer_active = false;
    }
}

winrt::fire_and_forget ConfigureDevelopmentOutputAsync(HWND owner) {
    try {
        constexpr wchar_t token[] = L"WAC-development-trace-folder";
        auto access_list = winrt::Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList();
        winrt::Windows::Storage::StorageFolder folder{nullptr};

        if (access_list.ContainsItem(token)) {
            try {
                folder = co_await access_list.GetFolderAsync(token);
            } catch (...) {
                try {
                    access_list.Remove(token);
                } catch (...) {
                }
            }
        }

        if (!folder) {
            winrt::Windows::Storage::Pickers::FolderPicker picker;
            picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
            picker.FileTypeFilter().Append(L"*");
            picker.as<::IInitializeWithWindow>()->Initialize(owner);
            folder = co_await picker.PickSingleFolderAsync();
            if (!folder) {
                Write(L"TRACE", L"development log folder selection cancelled; LocalState fallback remains active");
                co_return;
            }
            access_list.AddOrReplace(token, folder);
        }

        auto file = co_await folder.CreateFileAsync(
            L"wac-trace.log", winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);
        {
            auto& state = State();
            std::lock_guard lock(state.mutex);
            state.development_file = file;
        }
        Write(L"TRACE", L"development log destination=" + std::wstring{folder.Path().c_str()} + L"\\wac-trace.log");
        StartDevelopmentWriter();
    } catch (winrt::hresult_error const& error) {
        WriteHr(L"TRACE", L"development log folder setup failed", error.code());
        Write(L"TRACE", L"LocalState fallback remains active");
    } catch (...) {
        Write(L"TRACE", L"development log folder setup failed; LocalState fallback remains active");
    }
}
#endif

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
        {
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
        }
#ifdef _DEBUG
        StartDevelopmentWriter();
#endif
    } catch (...) {
        try {
            OutputDebugStringW(L"WAC TRACE FAILURE\n");
        } catch (...) {
        }
    }
}

#ifdef _DEBUG
void ConfigureDevelopmentOutput(HWND owner) noexcept {
    try {
        ConfigureDevelopmentOutputAsync(owner);
    } catch (...) {
        Write(L"TRACE", L"development log setup could not start; LocalState fallback remains active");
    }
}
#endif

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
