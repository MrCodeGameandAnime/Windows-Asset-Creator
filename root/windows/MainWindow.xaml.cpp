#include "MainWindow.xaml.h"
#include "ExplorerReveal.h"

#include <microsoft.ui.xaml.window.h>
#include <shobjidl_core.h>

#include <cwchar>
#include <memory>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#if __has_include("MainWindow.xaml.g.hpp")
#include "MainWindow.xaml.g.hpp"
#endif

namespace winrt::WindowsAssetCreator::implementation {
MainWindow::MainWindow() : view_model_impl_(winrt::make_self<AssetBoardViewModel>()),
                           view_model_(view_model_impl_.as<winrt::WindowsAssetCreator::AssetBoardViewModel>()) {
    InitializeComponent();
    Activated([this](winrt::Windows::Foundation::IInspectable const&,
                     winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const&) {
        if (Bindings) Bindings->Initialize();
    });
}
winrt::WindowsAssetCreator::AssetBoardViewModel MainWindow::ViewModel() const { return view_model_; }

winrt::fire_and_forget MainWindow::Browse_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {
    auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
    picker.FileTypeFilter().Append(L".png"); picker.FileTypeFilter().Append(L".jpg"); picker.FileTypeFilter().Append(L".jpeg");
    HWND hwnd{};
    auto window = get_strong().as<winrt::WindowsAssetCreator::MainWindow>();
    winrt::check_hresult(window.as<::IWindowNative>()->get_WindowHandle(&hwnd));
    winrt::check_hresult(picker.as<::IInitializeWithWindow>()->Initialize(hwnd));
    if (const auto file = co_await picker.PickSingleFileAsync()) GenerateFromSource(file.Path().c_str());
}
winrt::fire_and_forget MainWindow::SaveAs_Click(winrt::Windows::Foundation::IInspectable const&,
                                                 winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {
    auto picker = winrt::Windows::Storage::Pickers::FileSavePicker();
    auto extensions = winrt::single_threaded_vector<winrt::hstring>();
    extensions.Append(L".zip");
    picker.FileTypeChoices().Insert(L"ZIP archive", extensions);
    picker.SuggestedFileName(L"Windows-Assets.zip");

    HWND hwnd{};
    auto window = get_strong().as<winrt::WindowsAssetCreator::MainWindow>();
    winrt::check_hresult(window.as<::IWindowNative>()->get_WindowHandle(&hwnd));
    winrt::check_hresult(picker.as<::IInitializeWithWindow>()->Initialize(hwnd));

    const auto file = co_await picker.PickSaveFileAsync();
    if (!file || !view_model_impl_->BeginSave()) co_return;

    auto window_impl = get_strong();
    window_impl->Bindings->Update();
    const std::filesystem::path destination{file.Path().c_str()};
    const auto dispatcher_queue = DispatcherQueue();
    const auto view_model = view_model_impl_;
    co_await winrt::resume_background();
    auto result = std::make_shared<wac::OperationResult>(view_model->ExportZip(destination));
    dispatcher_queue.TryEnqueue([window_impl, view_model, result, destination] {
        if (!result->succeeded()) {
            auto diagnostic = result->diagnostics.empty()
                ? wac::Diagnostic{wac::Severity::error, wac::DiagnosticCode::zip_failure,
                                  L"The ZIP could not be saved.", destination.wstring()}
                : std::move(result->diagnostics.front());
            view_model->CompleteSaveFailure(std::move(diagnostic));
            window_impl->Bindings->Update();
            return;
        }

        view_model->CompleteSaveSuccess(destination, wac::RevealInExplorer(destination));
        window_impl->Bindings->Update();
    });
}
void MainWindow::DropSurface_DragOver(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const& args) {
    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy);
}
winrt::fire_and_forget MainWindow::DropSurface_Drop(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const& args) {
    const auto view = args.DataView();
    if (!view.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems())) { ShowUnsupportedInput(); co_return; }
    const auto items = co_await view.GetStorageItemsAsync();
    if (items.Size() != 1) { ShowUnsupportedInput(); co_return; }
    if (const auto file = items.GetAt(0).try_as<winrt::Windows::Storage::StorageFile>()) GenerateFromSource(file.Path().c_str()); else ShowUnsupportedInput();
}
winrt::fire_and_forget MainWindow::GenerateFromSource(std::filesystem::path source) {
    const auto extension = source.extension().wstring();
    if (_wcsicmp(extension.c_str(), L".png") != 0 && _wcsicmp(extension.c_str(), L".jpg") != 0 &&
        _wcsicmp(extension.c_str(), L".jpeg") != 0) {
        ShowUnsupportedInput();
        Bindings->Update();
        co_return;
    }
    view_model_impl_->BeginGeneration();
    auto window = get_strong();
    window->Bindings->Update();
    const auto dispatcher_queue = DispatcherQueue();
    const auto view_model = view_model_impl_;
    co_await winrt::resume_background();
    auto result = std::make_shared<wac::GenerationResult<wac::GenerationSession>>(
        wac::AssetGenerator{}.Generate(source));
    dispatcher_queue.TryEnqueue([window, view_model, result] {
        if (result->succeeded()) view_model->CompleteGeneration(std::move(*result->value));
        else view_model->CompleteFailure(std::move(result->diagnostics));
        window->Bindings->Update();
    });
}
void MainWindow::ShowUnsupportedInput() {
    view_model_impl_->CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::unsupported_image,
                                       L"Choose one PNG or JPEG image.", L""}});
}
}
