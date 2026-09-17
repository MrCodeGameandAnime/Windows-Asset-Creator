#include "ExplorerReveal.h"
#include "Diagnostics/Trace.h"

#include <shobjidl_core.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>

namespace wac {
winrt::Windows::Foundation::IAsyncOperation<bool> RevealInExplorerAsync(
    winrt::Windows::Storage::StorageFile const& saved_file,
    HWND owner) {
    try {
        const std::filesystem::path saved_path{saved_file.Path().c_str()};
        const auto parent_path = saved_path.parent_path();
        wac::trace::Write(L"EXPLORER", L"RevealInExplorerAsync destination=" + saved_path.wstring());
        wac::trace::Write(L"EXPLORER", L"parent path=" + parent_path.wstring());
        wac::trace::Write(L"EXPLORER", L"selected StorageFile name=" + std::wstring{saved_file.Name().c_str()});
        wac::trace::Write(L"EXPLORER", L"selected StorageFile path=" + std::wstring{saved_file.Path().c_str()});

        auto options = winrt::Windows::System::FolderLauncherOptions();
        options.ItemsToSelect().Append(saved_file);
        const auto initialize = options.as<::IInitializeWithWindow>();
        const auto initialize_result = initialize->Initialize(owner);
        wac::trace::WriteHr(L"EXPLORER", L"FolderLauncherOptions::Initialize", initialize_result);
        winrt::check_hresult(initialize_result);

        wac::trace::Write(L"EXPLORER", L"LaunchFolderPathAsync BEGIN");
        const auto launched = co_await winrt::Windows::System::Launcher::LaunchFolderPathAsync(
            parent_path.wstring(), options);
        wac::trace::Write(L"EXPLORER", L"LaunchFolderPathAsync result=" +
                                   std::wstring{launched ? L"true" : L"false"});
        co_return launched;
    } catch (winrt::hresult_error const& error) {
        wac::trace::WriteHr(L"EXPLORER", L"LaunchFolderPathAsync failure", error.code());
        wac::trace::Write(L"EXPLORER", L"LaunchFolderPathAsync message=" + std::wstring{error.message().c_str()});
        co_return false;
    } catch (...) {
        wac::trace::Write(L"EXPLORER", L"LaunchFolderPathAsync unknown failure");
        co_return false;
    }
}
}
