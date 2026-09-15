#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::WindowsAssetCreator::implementation {
MainWindow::MainWindow() : view_model_(winrt::make<AssetBoardViewModel>()) {
    InitializeComponent();
}
winrt::WindowsAssetCreator::AssetBoardViewModel MainWindow::ViewModel() const { return view_model_; }
}
