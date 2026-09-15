#pragma once

#include "AssetBoardViewModel.h"
#include "MainWindow.g.h"

namespace winrt::WindowsAssetCreator::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        AssetBoardViewModel& ViewModel() noexcept;

    private:
        winrt::com_ptr<AssetBoardViewModel> view_model_;
    };
}

namespace winrt::WindowsAssetCreator::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
