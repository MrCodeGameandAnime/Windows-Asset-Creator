#pragma once

#include "AssetBoardViewModel.h"
#include "MainWindow.g.h"

namespace winrt::WindowsAssetCreator::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        winrt::WindowsAssetCreator::AssetBoardViewModel ViewModel() const;
        void RefreshBindings();
        winrt::fire_and_forget Browse_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        winrt::fire_and_forget SaveAs_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void DropSurface_DragOver(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const&);
        winrt::fire_and_forget DropSurface_Drop(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const&);

    private:
        winrt::fire_and_forget GenerateFromSource(std::filesystem::path source);
        void ShowUnsupportedInput();
        winrt::WindowsAssetCreator::AssetBoardViewModel view_model_{nullptr};
        winrt::com_ptr<AssetBoardViewModel> view_model_impl_;
    };
}

namespace winrt::WindowsAssetCreator::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
