#pragma once

#include "MainWindow.g.h"

namespace winrt::WindowsAssetCreator::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
    };
}

namespace winrt::WindowsAssetCreator::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
