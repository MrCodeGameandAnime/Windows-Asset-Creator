#include "AssetBoardViewModel.h"

namespace winrt::WindowsAssetCreator::implementation {
AssetBoardViewModel::AssetBoardViewModel()
    : groups_(winrt::single_threaded_observable_vector<winrt::Windows::Foundation::IInspectable>()) {}

winrt::hstring AssetBoardViewModel::SourceName() const { return L"No source selected"; }
winrt::hstring AssetBoardViewModel::SourceDimensions() const { return L"Choose a PNG or JPEG to begin."; }
winrt::hstring AssetBoardViewModel::SourceFramingNote() const { return L"Artwork is centered on a transparent square and never cropped."; }
winrt::hstring AssetBoardViewModel::ValidationText() const {
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
winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable>
AssetBoardViewModel::Groups() const noexcept { return groups_; }

void AssetBoardViewModel::BeginGeneration() { state_.BeginGeneration(); NotifyChanged(); }
void AssetBoardViewModel::CompleteGeneration(wac::GenerationSession session) { state_.CompleteGeneration(std::move(session)); NotifyChanged(); }
void AssetBoardViewModel::CompleteFailure(std::vector<wac::Diagnostic> diagnostics) { state_.CompleteFailure(std::move(diagnostics)); NotifyChanged(); }

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
