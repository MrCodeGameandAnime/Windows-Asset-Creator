#pragma once

#include <windows.h>

#include <filesystem>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>

namespace wac {
winrt::Windows::Foundation::IAsyncOperation<bool> RevealInExplorerAsync(
    winrt::Windows::Storage::StorageFile const& saved_file,
    HWND owner);
}
