#include "ExplorerReveal.h"

#include <windows.h>

#pragma comment(lib, "shell32.lib")

namespace wac {
bool RevealInExplorer(std::filesystem::path const& saved_zip) noexcept {
    const auto arguments = L"/select,\"" + saved_zip.wstring() + L"\"";
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr,
                                                     SW_SHOWNORMAL)) > 32;
}
}
