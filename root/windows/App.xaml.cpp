#include "App.xaml.h"
#include "Diagnostics/Trace.h"
#include "MainWindow.xaml.h"

#ifdef _DEBUG
#include <microsoft.ui.xaml.window.h>
#include <shellapi.h>
#endif

#include <string_view>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace {
#ifdef _DEBUG
bool DiagnosticSwitchRequested() noexcept {
    int argument_count = 0;
    auto arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (!arguments) return false;

    bool requested = false;
    for (int index = 1; index < argument_count; ++index) {
        if (std::wstring_view{arguments[index]} == L"--wac-diagnostics") {
            requested = true;
            break;
        }
    }
    LocalFree(arguments);
    return requested;
}
#endif
}

namespace winrt::WindowsAssetCreator::implementation {
App::App() {
#ifdef _DEBUG
    if (DiagnosticSwitchRequested()) wac::trace::EnableDiagnostics();
#endif
    wac::trace::Write(L"APP", L"App::App ENTER");
    wac::trace::Write(L"APP", L"InitializeComponent BEGIN");
    InitializeComponent();
    wac::trace::Write(L"APP", L"InitializeComponent END");
}

void App::OnLaunched(LaunchActivatedEventArgs const&) {
    wac::trace::Write(L"APP", L"App::OnLaunched ENTER");
    wac::trace::Write(L"APP", L"MainWindow creation BEGIN");
    const auto main_window = make<MainWindow>();
    wac::trace::Write(L"APP", L"MainWindow creation END");
    window_ = main_window;
    wac::trace::Write(L"WINDOW", L"Window::Activate BEGIN");
    window_.Activate();
    wac::trace::Write(L"WINDOW", L"Window::Activate END");
    const auto main_window_impl = get_self<MainWindow>(main_window);
    main_window_impl->InitializeBindings();
    main_window_impl->RefreshBindings();
#ifdef _DEBUG
    if (wac::trace::DiagnosticsEnabled()) {
        HWND owner = nullptr;
        const auto owner_result = window_.as<::IWindowNative>()->get_WindowHandle(&owner);
        wac::trace::WriteHr(L"TRACE", L"IWindowNative::get_WindowHandle for development log picker", owner_result);
        if (SUCCEEDED(owner_result)) wac::trace::ConfigureDevelopmentOutput(owner);
    }
#endif
    wac::trace::Write(L"APP", L"App::OnLaunched EXIT");
}
}
