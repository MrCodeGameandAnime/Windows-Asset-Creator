#include "ExplorerReveal.h"
#include "Diagnostics/Trace.h"

#include <windows.h>

#pragma comment(lib, "shell32.lib")

namespace wac {
bool RevealInExplorer(std::filesystem::path const& saved_zip) noexcept {
    wac::trace::Write(L"EXPLORER", L"RevealInExplorer destination=" + saved_zip.wstring());
    const auto arguments = L"/select,\"" + saved_zip.wstring() + L"\"";
    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr,
                                                                 SW_SHOWNORMAL));
    wac::trace::Write(L"EXPLORER", L"ShellExecuteW return=" + std::to_wstring(result) +
                               (result > 32 ? L" success" : L" failure"));
    return result > 32;
}
}
