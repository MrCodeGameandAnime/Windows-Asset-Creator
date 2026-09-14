#pragma once

#include "App.xaml.g.h"

namespace winrt::WindowsAssetCreator::implementation {
struct App : AppT<App> {
    App();
    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);

private:
    Microsoft::UI::Xaml::Window window_{ nullptr };
};
}

namespace winrt::WindowsAssetCreator::factory_implementation {
struct App : AppT<App, implementation::App> {};
}
