#include "App.xaml.h"
#include "Diagnostics/Trace.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::WindowsAssetCreator::implementation {
App::App() {
    wac::trace::Initialize();
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
    get_self<MainWindow>(main_window)->RefreshBindings();
    wac::trace::Write(L"APP", L"App::OnLaunched EXIT");
}
}
