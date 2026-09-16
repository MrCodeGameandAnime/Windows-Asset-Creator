#include "AssetBoardViewModel.h"
#include "../src/Diagnostics/TraceSink.h"

#if __has_include("AssetBoardItemViewModel.g.cpp")
#include "AssetBoardItemViewModel.g.cpp"
#endif
#if __has_include("AssetBoardGroupViewModel.g.cpp")
#include "AssetBoardGroupViewModel.g.cpp"
#endif
#if __has_include("AssetBoardViewModel.g.cpp")
#include "AssetBoardViewModel.g.cpp"
#endif

#include <format>

namespace winrt::WindowsAssetCreator::implementation {
AssetBoardItemViewModel::AssetBoardItemViewModel(winrt::hstring label, winrt::hstring dimensions,
                                                 winrt::hstring preview_path)
    : label_(std::move(label)), dimensions_(std::move(dimensions)), preview_path_(std::move(preview_path)) {}
winrt::hstring AssetBoardItemViewModel::Label() const { return label_; }
winrt::hstring AssetBoardItemViewModel::Dimensions() const { return dimensions_; }
winrt::hstring AssetBoardItemViewModel::PreviewPath() const { return preview_path_; }

AssetBoardGroupViewModel::AssetBoardGroupViewModel(
    winrt::hstring title,
    winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets)
    : title_(std::move(title)), assets_(std::move(assets)) {}
winrt::hstring AssetBoardGroupViewModel::Title() const { return title_; }
winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardItemViewModel>
AssetBoardGroupViewModel::Assets() const { return assets_; }

AssetBoardViewModel::AssetBoardViewModel()
    : groups_(winrt::single_threaded_observable_vector<winrt::WindowsAssetCreator::AssetBoardGroupViewModel>()) {}
winrt::hstring AssetBoardViewModel::SourceName() const { return L"No source selected"; }
winrt::hstring AssetBoardViewModel::SourceDimensions() const { return L"Choose a PNG or JPEG to begin."; }
winrt::hstring AssetBoardViewModel::SourceFramingNote() const { return L"Artwork is centered on a transparent square and never cropped."; }
winrt::hstring AssetBoardViewModel::ValidationText() const {
    if (!save_status_.empty()) return save_status_;
    return state_.phase() == wac::BoardPhase::ready ? L"69 PNG + 1 ICO ready" : L"No generated assets yet.";
}
winrt::hstring AssetBoardViewModel::ErrorText() const {
    if (state_.diagnostics().empty()) return {};
    return winrt::hstring{state_.diagnostics().front().message};
}
bool AssetBoardViewModel::IsIdle() const noexcept { return state_.phase() == wac::BoardPhase::idle; }
bool AssetBoardViewModel::IsBusy() const noexcept { return state_.phase() == wac::BoardPhase::processing || state_.phase() == wac::BoardPhase::saving; }
bool AssetBoardViewModel::HasError() const noexcept { return state_.phase() == wac::BoardPhase::error; }
bool AssetBoardViewModel::CanSave() const noexcept { return state_.can_save(); }
winrt::Microsoft::UI::Xaml::Visibility AssetBoardViewModel::EmptyDropTargetVisibility() const noexcept {
    return IsIdle() ? winrt::Microsoft::UI::Xaml::Visibility::Visible : winrt::Microsoft::UI::Xaml::Visibility::Collapsed;
}
winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardGroupViewModel>
AssetBoardViewModel::Groups() const { return groups_.GetView(); }

void AssetBoardViewModel::BeginGeneration() {
    save_status_.clear();
    state_.BeginGeneration();
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"diagnostics=0 groups=0 generated_assets=0 CanSave=false");
    NotifyChanged();
}
void AssetBoardViewModel::CompleteGeneration(wac::GenerationSession session) {
    state_.CompleteGeneration(std::move(session));
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"diagnostics=" + std::to_wstring(state_.diagnostics().size()) +
                                  L" groups=" + std::to_wstring(state_.groups().size()) +
                                  L" generated_assets=" + std::to_wstring(state_.session()->preview_assets().size()) +
                                  L" CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}
void AssetBoardViewModel::CompleteFailure(std::vector<wac::Diagnostic> diagnostics) {
    save_status_.clear();
    state_.CompleteFailure(std::move(diagnostics));
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"diagnostics=" + std::to_wstring(state_.diagnostics().size()) +
                                  L" groups=0 generated_assets=0 CanSave=false");
    NotifyChanged();
}
bool AssetBoardViewModel::BeginSave() {
    if (!state_.can_save()) return false;
    save_status_.clear();
    state_.BeginSave();
    wac::trace_sink::Emit(L"STATE", L"save status=Saving CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
    return true;
}
wac::OperationResult AssetBoardViewModel::ExportZip(std::filesystem::path const& destination) const {
    const auto& session = state_.session();
    if (session) return session->ExportZip(destination);
    return {{{wac::Severity::error, wac::DiagnosticCode::zip_failure,
              L"No generated assets are available to save.", destination.wstring()}}};
}
void AssetBoardViewModel::CompleteSaveCancelled() {
    state_.CompleteSaveCancelled();
    wac::trace_sink::Emit(L"STATE", L"save status=Cancelled CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}
void AssetBoardViewModel::CompleteSaveSuccess(std::filesystem::path const& destination, bool explorer_opened) {
    state_.CompleteSaveSuccess();
    save_status_ = explorer_opened
        ? winrt::hstring{L"Saved " + destination.filename().wstring() + L"."}
        : winrt::hstring{L"Saved, but Explorer could not be opened."};
    wac::trace_sink::Emit(L"STATE", L"save status=Success CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}
void AssetBoardViewModel::CompleteSaveFailure(wac::Diagnostic diagnostic) {
    state_.CompleteSaveFailure(std::move(diagnostic));
    wac::trace_sink::Emit(L"STATE", L"save status=Failure diagnostics=" + std::to_wstring(state_.diagnostics().size()) +
                                  L" CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}

void AssetBoardViewModel::RefreshGroups() {
    groups_.Clear();
    for (const auto& group : state_.groups()) {
        auto assets = winrt::single_threaded_vector<winrt::WindowsAssetCreator::AssetBoardItemViewModel>();
        for (const auto& asset : group.assets) {
            const auto dimensions = std::format(L"{} × {} px", asset.size.width, asset.size.height);
            assets.Append(winrt::make<AssetBoardItemViewModel>(winrt::hstring{asset.label},
                                                                winrt::hstring{dimensions},
                                                                winrt::hstring{asset.staged_path.wstring()}));
        }
        groups_.Append(winrt::make<AssetBoardGroupViewModel>(winrt::hstring{group.title}, assets.GetView()));
    }
}
winrt::event_token AssetBoardViewModel::PropertyChanged(
    winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler) {
    return property_changed_.add(handler);
}
void AssetBoardViewModel::PropertyChanged(winrt::event_token const& token) noexcept {
    property_changed_.remove(token);
}
void AssetBoardViewModel::NotifyChanged() {
    property_changed_(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L""});
}
}
