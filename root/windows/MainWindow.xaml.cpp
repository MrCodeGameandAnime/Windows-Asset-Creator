#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::WindowsAssetCreator::implementation {
MainWindow::MainWindow() : view_model_(winrt::make_self<AssetBoardViewModel>()) {
    InitializeComponent();
    Root().DataContext(view_model_.as<winrt::Windows::Foundation::IInspectable>());
}
AssetBoardViewModel& MainWindow::ViewModel() noexcept { return *view_model_; }
}
