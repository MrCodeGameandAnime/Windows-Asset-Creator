#pragma once

#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::WindowsAssetCreator::implementation {
template <typename D, typename... Interfaces>
struct MainWindow_base : winrt::Microsoft::UI::Xaml::WindowT<D, Interfaces...> {
    using base_type = MainWindow_base;
    using class_type = D;
};
}

#include "MainWindow.xaml.g.h"

namespace winrt::WindowsAssetCreator::implementation {
struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
};
}
