#pragma once

#include "AssetBoardViewModel.h"
#include "MainWindow.g.h"

namespace winrt::WindowsAssetCreator::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        winrt::WindowsAssetCreator::AssetBoardViewModel ViewModel() const;

    private:
        winrt::WindowsAssetCreator::AssetBoardViewModel view_model_{nullptr};
    };
}

namespace winrt::WindowsAssetCreator::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
