#include "MainWindow.xaml.h"
#include "Diagnostics/Trace.h"
#include "ExplorerReveal.h"
#include "../src/Diagnostics/TraceSink.h"

#include <microsoft.ui.xaml.window.h>
#include <shobjidl_core.h>

#include <cwchar>
#include <exception>
#include <memory>
#include <string>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#if __has_include("MainWindow.xaml.g.hpp")
#include "MainWindow.xaml.g.hpp"
#endif

namespace {
std::wstring OperationTag(std::wstring_view prefix, uint64_t id) {
    return std::wstring{prefix} + L"#" + std::to_wstring(id);
}

std::wstring NarrowException(std::string_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const auto character : value) result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(character)));
    return result;
}

void TraceHResultException(std::wstring_view area, std::wstring_view operation,
                           winrt::hresult_error const& error) noexcept {
    wac::trace::WriteHr(area, operation, error.code());
    wac::trace::Write(area, std::wstring{operation} + L" message=" + error.message().c_str());
}

void TraceStdException(std::wstring_view area, std::wstring_view operation,
                       std::exception const& error) noexcept {
    wac::trace::Write(area, std::wstring{operation} + L" std::exception=" + NarrowException(error.what()));
}
}

namespace winrt::WindowsAssetCreator::implementation {
MainWindow::MainWindow() : view_model_impl_(winrt::make_self<AssetBoardViewModel>()),
                           view_model_(view_model_impl_.as<winrt::WindowsAssetCreator::AssetBoardViewModel>()) {
    wac::trace::Write(L"WINDOW", L"MainWindow ctor ENTER");
    wac::trace::Write(L"XAML", L"InitializeComponent BEGIN");
    InitializeComponent();
    wac::trace::Write(L"XAML", L"InitializeComponent END");
    wac::trace::Write(L"WINDOW", L"MainWindow ctor EXIT");
}
winrt::WindowsAssetCreator::AssetBoardViewModel MainWindow::ViewModel() const { return view_model_; }
void MainWindow::RefreshBindings() {
    wac::trace_sink::Emit(L"XAML", L"Bindings->Update BEGIN");
    if (Bindings) Bindings->Update();
    wac::trace_sink::Emit(L"XAML", L"Bindings->Update END");
}

winrt::fire_and_forget MainWindow::Browse_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {
    try {
        wac::trace::Write(L"PICKER", L"Browse_Click ENTER");
        wac::trace::Write(L"PICKER", L"FileOpenPicker construction");
        auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
        picker.FileTypeFilter().Append(L".png");
        picker.FileTypeFilter().Append(L".jpg");
        picker.FileTypeFilter().Append(L".jpeg");

        HWND hwnd{};
        auto window = get_strong().as<winrt::WindowsAssetCreator::MainWindow>();
        const auto window_result = window.as<::IWindowNative>()->get_WindowHandle(&hwnd);
        wac::trace::WriteHr(L"PICKER", L"IWindowNative::get_WindowHandle", window_result);
        winrt::check_hresult(window_result);
        const auto picker_result = picker.as<::IInitializeWithWindow>()->Initialize(hwnd);
        wac::trace::WriteHr(L"PICKER", L"FileOpenPicker::Initialize", picker_result);
        winrt::check_hresult(picker_result);

        wac::trace::Write(L"PICKER", L"PickSingleFileAsync BEGIN");
        const auto file = co_await picker.PickSingleFileAsync();
        if (!file) {
            wac::trace::Write(L"PICKER", L"PickSingleFileAsync CANCELLED");
            co_return;
        }

        wac::trace::Write(L"PICKER", std::wstring{L"PickSingleFileAsync SELECTED name="} + file.Name().c_str());
        wac::trace::Write(L"PICKER", std::wstring{L"selected path="} + file.Path().c_str());
        wac::trace::Write(L"PICKER", L"detected extension=" + std::filesystem::path{file.Path().c_str()}.extension().wstring());
        wac::trace::Write(L"PICKER", L"handoff into generation");
        GenerateFromStorageFile(file);
    } catch (winrt::hresult_error const& error) {
        TraceHResultException(L"PICKER", L"Browse_Click exception", error);
        throw;
    } catch (std::exception const& error) {
        TraceStdException(L"PICKER", L"Browse_Click exception", error);
        throw;
    }
}
winrt::fire_and_forget MainWindow::SaveAs_Click(winrt::Windows::Foundation::IInspectable const&,
                                                 winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {
    try {
        wac::trace::Write(L"PICKER", L"SaveAs_Click ENTER");
        wac::trace::Write(L"PICKER", L"FileSavePicker construction");
        auto picker = winrt::Windows::Storage::Pickers::FileSavePicker();
        auto extensions = winrt::single_threaded_vector<winrt::hstring>();
        extensions.Append(L".zip");
        picker.FileTypeChoices().Insert(L"ZIP archive", extensions);
        picker.SuggestedFileName(L"Windows-Assets.zip");

        HWND hwnd{};
        auto window = get_strong().as<winrt::WindowsAssetCreator::MainWindow>();
        const auto window_result = window.as<::IWindowNative>()->get_WindowHandle(&hwnd);
        wac::trace::WriteHr(L"PICKER", L"IWindowNative::get_WindowHandle", window_result);
        winrt::check_hresult(window_result);
        const auto picker_result = picker.as<::IInitializeWithWindow>()->Initialize(hwnd);
        wac::trace::WriteHr(L"PICKER", L"FileSavePicker::Initialize", picker_result);
        winrt::check_hresult(picker_result);

        wac::trace::Write(L"PICKER", L"PickSaveFileAsync BEGIN");
        const auto file = co_await picker.PickSaveFileAsync();
        if (!file) {
            wac::trace::Write(L"PICKER", L"PickSaveFileAsync CANCELLED");
            co_return;
        }

        const auto operation_id = wac::trace::NextOperationId();
        const auto tag = OperationTag(L"SAVE", operation_id);
        const std::filesystem::path destination{file.Path().c_str()};
        wac::trace::Write(L"PICKER", tag + L" selected destination name=" + std::wstring{file.Name().c_str()});
        wac::trace::Write(L"PICKER", tag + L" selected destination path=" + destination.wstring());
        wac::trace_sink::SetOperationTag(tag);
        const auto began = view_model_impl_->BeginSave();
        wac::trace_sink::Emit(L"STATE", began ? L"BeginSave result=true" : L"BeginSave result=false");
        wac::trace_sink::ClearOperationTag();
        if (!began) co_return;

        auto window_impl = get_strong();
        window_impl->RefreshBindings();
        const auto dispatcher_queue = DispatcherQueue();
        wac::trace::Write(L"DISPATCH", tag + L" DispatcherQueue captured");
        const auto view_model = view_model_impl_;
        co_await winrt::resume_background();
        {
            wac::trace_sink::OperationTagScope trace_scope(tag);
            wac::trace_sink::Emit(L"GENERATE", L"background export ENTER");
            auto result = std::make_shared<wac::OperationResult>(view_model->ExportZip(destination));
            wac::trace_sink::Emit(result->succeeded() ? L"GENERATE" : L"GENERATE",
                                  result->succeeded() ? L"ExportZip result=success" : L"ExportZip result=failure");
            const auto enqueued = dispatcher_queue.TryEnqueue([window_impl, view_model, result, destination, tag] {
                try {
                    wac::trace_sink::SetOperationTag(tag);
                    wac::trace_sink::Emit(L"UI", L"completion callback ENTER");
                    if (!result->succeeded()) {
                        auto diagnostic = result->diagnostics.empty()
                            ? wac::Diagnostic{wac::Severity::error, wac::DiagnosticCode::zip_failure,
                                              L"The ZIP could not be saved.", destination.wstring()}
                            : std::move(result->diagnostics.front());
                        view_model->CompleteSaveFailure(std::move(diagnostic));
                    } else {
                        view_model->CompleteSaveSuccess(destination, wac::RevealInExplorer(destination));
                    }
                    window_impl->RefreshBindings();
                    wac::trace_sink::ClearOperationTag();
                } catch (winrt::hresult_error const& error) {
                    TraceHResultException(L"UI", L"Save completion callback exception", error);
                    wac::trace_sink::ClearOperationTag();
                    throw;
                } catch (std::exception const& error) {
                    TraceStdException(L"UI", L"Save completion callback exception", error);
                    wac::trace_sink::ClearOperationTag();
                    throw;
                }
            });
            wac::trace_sink::Emit(enqueued ? L"DISPATCH" : L"DISPATCH",
                                  enqueued ? L"TryEnqueue result=true" : L"TryEnqueue result=false");
        }
    } catch (winrt::hresult_error const& error) {
        TraceHResultException(L"PICKER", L"SaveAs_Click exception", error);
        throw;
    } catch (std::exception const& error) {
        TraceStdException(L"PICKER", L"SaveAs_Click exception", error);
        throw;
    }
}
void MainWindow::DropSurface_DragOver(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const& args) {
    wac::trace::Write(L"DROP", L"DragOver ENTER");
    args.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy);
}
winrt::fire_and_forget MainWindow::DropSurface_Drop(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs const& args) {
    try {
        wac::trace::Write(L"DROP", L"Drop ENTER");
        const auto view = args.DataView();
        const auto has_storage_items = view.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems());
        wac::trace::Write(L"DROP", has_storage_items ? L"StorageItems present=true" : L"StorageItems present=false");
        if (!has_storage_items) {
            wac::trace::Write(L"DROP", L"rejected: StorageItems absent");
            ShowUnsupportedInput();
            co_return;
        }
        const auto items = co_await view.GetStorageItemsAsync();
        wac::trace::Write(L"DROP", L"StorageItems count=" + std::to_wstring(items.Size()));
        if (items.Size() != 1) {
            wac::trace::Write(L"DROP", L"rejected: expected exactly one item");
            ShowUnsupportedInput();
            co_return;
        }
        if (const auto file = items.GetAt(0).try_as<winrt::Windows::Storage::StorageFile>()) {
            wac::trace::Write(L"DROP", std::wstring{L"selected file name="} + file.Name().c_str());
            wac::trace::Write(L"DROP", std::wstring{L"selected file path="} + file.Path().c_str());
            GenerateFromStorageFile(file);
        } else {
            wac::trace::Write(L"DROP", L"rejected: selected storage item is a folder");
            ShowUnsupportedInput();
        }
    } catch (winrt::hresult_error const& error) {
        TraceHResultException(L"DROP", L"Drop exception", error);
        throw;
    } catch (std::exception const& error) {
        TraceStdException(L"DROP", L"Drop exception", error);
        throw;
    }
}
winrt::fire_and_forget MainWindow::GenerateFromStorageFile(winrt::Windows::Storage::StorageFile source) {
    const auto operation_id = wac::trace::NextOperationId();
    const auto tag = OperationTag(L"GEN", operation_id);
    try {
        const auto original_name = std::wstring{source.Name().c_str()};
        const auto original_path = std::wstring{source.Path().c_str()};
        const std::filesystem::path source_path{original_path};
        wac::trace::Write(L"PICKER", tag + L" original StorageFile name=" + original_name);
        wac::trace::Write(L"PICKER", tag + L" original path=" + original_path);
        wac::trace::Write(L"GENERATE", tag + L" GenerateFromStorageFile ENTER path=" + original_path);
        const auto extension = source_path.extension().wstring();
        wac::trace::Write(L"GENERATE", tag + L" detected extension=" + extension);
        if (_wcsicmp(extension.c_str(), L".png") != 0 && _wcsicmp(extension.c_str(), L".jpg") != 0 &&
            _wcsicmp(extension.c_str(), L".jpeg") != 0) {
            wac::trace_sink::SetOperationTag(tag);
            wac::trace_sink::Emit(L"STATE", L"rejected unsupported input");
            ShowUnsupportedInput();
            wac::trace_sink::ClearOperationTag();
            co_return;
        }

        wac::trace_sink::SetOperationTag(tag);
        view_model_impl_->BeginGeneration();
        wac::trace_sink::Emit(L"STATE", L"BeginGeneration requested");
        wac::trace_sink::ClearOperationTag();

        auto window = get_strong();
        window->RefreshBindings();
        const auto dispatcher_queue = DispatcherQueue();
        wac::trace::Write(L"DISPATCH", tag + L" DispatcherQueue captured");
        const auto view_model = view_model_impl_;

        winrt::Windows::Storage::StorageFile temp_source{nullptr};
        std::filesystem::path temp_path;
        const auto temp_folder = winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder();
        const auto temp_folder_path = std::wstring{temp_folder.Path().c_str()};
        const auto temp_name = std::wstring{L"source-"} + std::to_wstring(operation_id) + extension;
        wac::trace::Write(L"STORAGE", tag + L" TemporaryFolder path=" + temp_folder_path);
        wac::trace::Write(L"STORAGE", tag + L" brokered source copy BEGIN destination=" + temp_name);
        try {
            temp_source = co_await source.CopyAsync(
                temp_folder, temp_name,
                winrt::Windows::Storage::NameCollisionOption::GenerateUniqueName);
            temp_path = std::filesystem::path{temp_source.Path().c_str()};
            wac::trace::Write(L"STORAGE", tag + L" brokered source copy SUCCESS path=" + temp_path.wstring());
        } catch (winrt::hresult_error const& error) {
            wac::trace::WriteHr(L"STORAGE", tag + L" brokered source copy FAILURE", error.code());
            wac::trace::Write(L"STORAGE", tag + L" brokered source copy message=" + std::wstring{error.message().c_str()});
            wac::trace::Write(L"STORAGE", tag + L" original path=" + original_path);
            wac::trace::Write(L"STORAGE", tag + L" intended temp destination=" + temp_folder_path + L"\\" + temp_name);
            wac::trace_sink::SetOperationTag(tag);
            view_model_impl_->CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::io_failure,
                                                L"The selected image could not be accessed or read.",
                                                L"The app could not copy the selected image into its temporary workspace."}});
            wac::trace_sink::Emit(L"STATE", L"brokered source copy failure applied");
            window->RefreshBindings();
            wac::trace_sink::ClearOperationTag();
            co_return;
        } catch (std::exception const& error) {
            wac::trace::Write(L"STORAGE", tag + L" brokered source copy std::exception=" + NarrowException(error.what()));
            wac::trace::Write(L"STORAGE", tag + L" original path=" + original_path);
            wac::trace::Write(L"STORAGE", tag + L" intended temp destination=" + temp_folder_path + L"\\" + temp_name);
            wac::trace_sink::SetOperationTag(tag);
            view_model_impl_->CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::io_failure,
                                                L"The selected image could not be accessed or read.",
                                                L"The app could not copy the selected image into its temporary workspace."}});
            wac::trace_sink::Emit(L"STATE", L"brokered source copy failure applied");
            window->RefreshBindings();
            wac::trace_sink::ClearOperationTag();
            co_return;
        }

        co_await winrt::resume_background();
        wac::trace::Write(L"DISPATCH", tag + L" background switch complete");
        std::shared_ptr<wac::GenerationResult<wac::GenerationSession>> result;
        {
            wac::trace_sink::OperationTagScope trace_scope(tag);
            wac::trace_sink::Emit(L"GENERATE", L"background ENTER");
            result = std::make_shared<wac::GenerationResult<wac::GenerationSession>>(
                wac::AssetGenerator{}.Generate(temp_path));
            wac::trace_sink::Emit(L"GENERATE", result->succeeded() ? L"background result ready success" : L"background result ready failure");
        }

        try {
            co_await temp_source.DeleteAsync();
            wac::trace::Write(L"STORAGE", tag + L" temporary source cleanup SUCCESS path=" + temp_path.wstring());
        } catch (winrt::hresult_error const& error) {
            wac::trace::WriteHr(L"STORAGE", tag + L" temporary source cleanup FAILURE", error.code());
            wac::trace::Write(L"STORAGE", tag + L" temporary source cleanup message=" + std::wstring{error.message().c_str()});
        } catch (std::exception const& error) {
            wac::trace::Write(L"STORAGE", tag + L" temporary source cleanup std::exception=" + NarrowException(error.what()));
        }

        const auto enqueued = dispatcher_queue.TryEnqueue([window, view_model, result, tag] {
            try {
                wac::trace_sink::SetOperationTag(tag);
                wac::trace_sink::Emit(L"UI", L"completion callback ENTER");
                if (result->succeeded()) {
                    view_model->CompleteGeneration(std::move(*result->value));
                    wac::trace_sink::Emit(L"STATE", L"CompleteGeneration applied");
                } else {
                    wac::trace_sink::Emit(L"STATE", L"CompleteFailure applied diagnostics=" + std::to_wstring(result->diagnostics.size()));
                    view_model->CompleteFailure(std::move(result->diagnostics));
                }
                window->RefreshBindings();
                wac::trace_sink::ClearOperationTag();
            } catch (winrt::hresult_error const& error) {
                TraceHResultException(L"UI", L"Generation completion callback exception", error);
                wac::trace_sink::ClearOperationTag();
                throw;
            } catch (std::exception const& error) {
                TraceStdException(L"UI", L"Generation completion callback exception", error);
                wac::trace_sink::ClearOperationTag();
                throw;
            }
        });
        wac::trace::Write(L"DISPATCH", tag + (enqueued ? L" TryEnqueue result=true" : L" TryEnqueue result=false"));
    } catch (winrt::hresult_error const& error) {
        TraceHResultException(L"GENERATE", tag + L" exception", error);
        throw;
    } catch (std::exception const& error) {
        TraceStdException(L"GENERATE", tag + L" exception", error);
        throw;
    }
}
void MainWindow::ShowUnsupportedInput() {
    wac::trace_sink::Emit(L"STATE", L"CompleteFailure unsupported_image");
    view_model_impl_->CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::unsupported_image,
                                       L"Choose one PNG or JPEG image.", L""}});
}
}
