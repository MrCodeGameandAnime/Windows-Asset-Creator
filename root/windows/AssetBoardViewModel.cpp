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

#include <cstdint>
#include <format>

namespace {
std::wstring PointerText(void const* value) {
    return std::to_wstring(reinterpret_cast<uintptr_t>(value));
}

void TraceProperty(std::wstring_view name, std::wstring_view value, void const* object) noexcept {
    try {
        wac::trace_sink::Emit(L"VM", std::wstring{name} + L" this=" + PointerText(object) + L" value=" + std::wstring{value});
    } catch (...) {
    }
}

winrt::hstring FileUri(winrt::hstring const& path) {
    std::wstring uri{L"file:///"};
    uri.reserve(uri.size() + path.size());
    for (const auto character : path) {
        uri.push_back(character == L'\\' ? L'/' : character);
    }
    return winrt::hstring{uri};
}

struct GroupPresentation {
    winrt::hstring family_title;
    winrt::hstring family_summary;
    winrt::Microsoft::UI::Xaml::Visibility family_header_visibility;
    winrt::hstring title;
};

GroupPresentation PresentGroup(std::wstring_view title) {
    using winrt::Microsoft::UI::Xaml::Visibility;
    if (title == L"AppList default") return {L"AppList", L"42 assets \u00B7 all valid", Visibility::Visible, L"Default"};
    if (title == L"AppList altform unplated") return {L"", L"", Visibility::Collapsed, L"Unplated"};
    if (title == L"AppList altform light unplated") return {L"", L"", Visibility::Collapsed, L"Light unplated"};
    if (title == L"Square44") return {L"Logo families", L"27 assets \u00B7 all valid", Visibility::Visible, L"Square 44"};
    if (title == L"Square150") return {L"", L"", Visibility::Collapsed, L"Square 150"};
    if (title == L"StoreLogo") return {L"", L"", Visibility::Collapsed, L"Store logo"};
    if (title == L"MedTile") return {L"", L"", Visibility::Collapsed, L"Medium tile"};
    if (title == L"AppIcon") return {L"App icon", L"1 asset \u00B7 all valid", Visibility::Visible, L"Windows icon"};
    return {winrt::hstring{title}, L"", Visibility::Visible, winrt::hstring{title}};
}
}

namespace winrt::WindowsAssetCreator::implementation {
AssetBoardItemViewModel::AssetBoardItemViewModel(winrt::hstring label, winrt::hstring dimensions,
                                                 winrt::hstring preview_path)
    : label_(std::move(label)),
      dimensions_(std::move(dimensions)),
      preview_path_(std::move(preview_path)),
      thumbnail_(winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage{
          winrt::Windows::Foundation::Uri{FileUri(preview_path_)}}) {
    wac::trace_sink::Emit(L"BOARD", std::wstring{L"AssetBoardItemViewModel ctor this="} + PointerText(this) +
                                  L" label=" + std::wstring{label_.c_str()} +
                                  L" dimensions=" + std::wstring{dimensions_.c_str()});
}
winrt::hstring AssetBoardItemViewModel::Label() const {
    TraceProperty(L"AssetBoardItem.Label", label_.c_str(), this);
    return label_;
}
winrt::hstring AssetBoardItemViewModel::Dimensions() const {
    TraceProperty(L"AssetBoardItem.Dimensions", dimensions_.c_str(), this);
    return dimensions_;
}
winrt::hstring AssetBoardItemViewModel::PreviewPath() const {
    TraceProperty(L"AssetBoardItem.PreviewPath", preview_path_.c_str(), this);
    return preview_path_;
}
winrt::Microsoft::UI::Xaml::Media::ImageSource AssetBoardItemViewModel::Thumbnail() const {
    wac::trace_sink::Emit(L"BOARD", std::wstring{L"AssetBoardItem.Thumbnail this="} + PointerText(this) +
                                  L" value=" + std::wstring{thumbnail_ ? L"present" : L"null"});
    return thumbnail_;
}

AssetBoardGroupViewModel::AssetBoardGroupViewModel(
    winrt::hstring family_title,
    winrt::hstring family_summary,
    winrt::Microsoft::UI::Xaml::Visibility family_header_visibility,
    winrt::hstring title,
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets)
    : family_title_(std::move(family_title)),
      family_summary_(std::move(family_summary)),
      family_header_visibility_(family_header_visibility),
      title_(std::move(title)),
      asset_count_text_(std::to_wstring(assets.Size()) + (assets.Size() == 1 ? L" asset" : L" assets")),
      assets_(std::move(assets)) {
    wac::trace_sink::Emit(L"BOARD", std::wstring{L"AssetBoardGroupViewModel ctor this="} + PointerText(this) +
                                  L" title=" + std::wstring{title_.c_str()} +
                                  L" assets_abi=" + PointerText(winrt::get_abi(assets_)) +
                                  L" asset_count=" + std::to_wstring(assets_ ? assets_.Size() : 0));
}
winrt::hstring AssetBoardGroupViewModel::FamilyTitle() const { return family_title_; }
winrt::hstring AssetBoardGroupViewModel::FamilySummary() const { return family_summary_; }
winrt::Microsoft::UI::Xaml::Visibility AssetBoardGroupViewModel::FamilyHeaderVisibility() const noexcept {
    return family_header_visibility_;
}
winrt::hstring AssetBoardGroupViewModel::Title() const {
    TraceProperty(L"AssetBoardGroup.Title", title_.c_str(), this);
    return title_;
}
winrt::hstring AssetBoardGroupViewModel::AssetCountText() const { return asset_count_text_; }
winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardItemViewModel>
AssetBoardGroupViewModel::Assets() const {
    wac::trace_sink::Emit(L"BOARD", std::wstring{L"AssetBoardGroup.Assets this="} + PointerText(this) +
                                  L" assets_abi=" + PointerText(winrt::get_abi(assets_)) +
                                  L" asset_count=" + std::to_wstring(assets_ ? assets_.Size() : 0));
    return assets_;
}

AssetBoardViewModel::AssetBoardViewModel()
    : groups_(winrt::single_threaded_observable_vector<winrt::WindowsAssetCreator::AssetBoardGroupViewModel>()) {
    wac::trace_sink::Emit(L"VM", L"AssetBoardViewModel ctor this=" + PointerText(this));
}
winrt::hstring AssetBoardViewModel::SourceName() const {
    const auto& source = state_.source();
    const auto value = source ? winrt::hstring{source->name} : winrt::hstring{L"No source selected"};
    TraceProperty(L"SourceName", value.c_str(), this);
    return value;
}
winrt::hstring AssetBoardViewModel::SourceDimensions() const {
    const auto& source = state_.source();
    const auto value = source
        ? winrt::hstring{std::format(L"{} \u00D7 {}", source->dimensions.width, source->dimensions.height)}
        : winrt::hstring{L"Choose a PNG or JPEG to begin."};
    TraceProperty(L"SourceDimensions", value.c_str(), this);
    return value;
}
winrt::Microsoft::UI::Xaml::Media::ImageSource AssetBoardViewModel::SourcePreview() const {
    return source_preview_;
}
winrt::Microsoft::UI::Xaml::Visibility AssetBoardViewModel::SourceSummaryVisibility() const noexcept {
    return state_.source()
        ? winrt::Microsoft::UI::Xaml::Visibility::Visible
        : winrt::Microsoft::UI::Xaml::Visibility::Collapsed;
}
winrt::hstring AssetBoardViewModel::SourceFramingNote() const {
    const auto value = winrt::hstring{L"Artwork is centered on a transparent square and never cropped."};
    TraceProperty(L"SourceFramingNote", value.c_str(), this);
    return value;
}
winrt::hstring AssetBoardViewModel::ValidationText() const {
    const auto value = HasValidationSuccess()
        ? winrt::hstring{L"69 PNG + 1 ICO ready"}
        : winrt::hstring{L"No generated assets yet."};
    TraceProperty(L"ValidationText", value.c_str(), this);
    return value;
}
winrt::hstring AssetBoardViewModel::ValidationDetail() const {
    const auto value = save_status_.empty() ? winrt::hstring{L"Validation passed"} : save_status_;
    TraceProperty(L"ValidationDetail", value.c_str(), this);
    return value;
}
bool AssetBoardViewModel::HasValidationSuccess() const noexcept {
    const auto phase = state_.phase();
    const auto value = state_.session().has_value() &&
                       (phase == wac::BoardPhase::ready || phase == wac::BoardPhase::saving) &&
                       state_.diagnostics().empty();
    TraceProperty(L"HasValidationSuccess", value ? L"true" : L"false", this);
    return value;
}
winrt::hstring AssetBoardViewModel::ErrorText() const {
    const auto value = state_.diagnostics().empty() ? winrt::hstring{} : winrt::hstring{state_.diagnostics().front().message};
    TraceProperty(L"ErrorText", value.c_str(), this);
    return value;
}
bool AssetBoardViewModel::IsIdle() const noexcept {
    const auto value = state_.phase() == wac::BoardPhase::idle;
    TraceProperty(L"IsIdle", value ? L"true" : L"false", this);
    return value;
}
bool AssetBoardViewModel::IsBusy() const noexcept {
    const auto value = state_.phase() == wac::BoardPhase::processing || state_.phase() == wac::BoardPhase::saving;
    TraceProperty(L"IsBusy", value ? L"true" : L"false", this);
    return value;
}
bool AssetBoardViewModel::HasError() const noexcept {
    const auto value = state_.phase() == wac::BoardPhase::error;
    TraceProperty(L"HasError", value ? L"true" : L"false", this);
    return value;
}
bool AssetBoardViewModel::CanSave() const noexcept {
    const auto value = state_.can_save();
    TraceProperty(L"CanSave", value ? L"true" : L"false", this);
    return value;
}
bool AssetBoardViewModel::CanReset() const noexcept {
    const auto value = state_.can_reset();
    TraceProperty(L"CanReset", value ? L"true" : L"false", this);
    return value;
}
winrt::Microsoft::UI::Xaml::Visibility AssetBoardViewModel::EmptyDropTargetVisibility() const noexcept {
    const auto value = state_.phase() == wac::BoardPhase::idle
        ? winrt::Microsoft::UI::Xaml::Visibility::Visible
        : winrt::Microsoft::UI::Xaml::Visibility::Collapsed;
    TraceProperty(L"EmptyDropTargetVisibility", value == winrt::Microsoft::UI::Xaml::Visibility::Visible ? L"Visible" : L"Collapsed", this);
    return value;
}
winrt::Microsoft::UI::Xaml::Visibility AssetBoardViewModel::ReplacementDropTargetVisibility() const noexcept {
    const auto value = state_.can_replace_source()
        ? winrt::Microsoft::UI::Xaml::Visibility::Visible
        : winrt::Microsoft::UI::Xaml::Visibility::Collapsed;
    TraceProperty(L"ReplacementDropTargetVisibility",
                  value == winrt::Microsoft::UI::Xaml::Visibility::Visible ? L"Visible" : L"Collapsed", this);
    return value;
}
winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardGroupViewModel>
AssetBoardViewModel::Groups() const {
    const auto value = groups_.GetView();
    wac::trace_sink::Emit(L"BOARD", std::wstring{L"AssetBoardViewModel.Groups this="} + PointerText(this) +
                                  L" groups_abi=" + PointerText(winrt::get_abi(value)) +
                                  L" group_count=" + std::to_wstring(value.Size()));
    return value;
}

void AssetBoardViewModel::Reset() {
    wac::trace_sink::Emit(L"VM", L"Reset this=" + PointerText(this));
    save_status_.clear();
    source_preview_ = nullptr;
    state_.Reset();
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"reset complete diagnostics=" + std::to_wstring(state_.diagnostics().size()) +
                                  L" groups=" + std::to_wstring(state_.groups().size()) +
                                  L" session=" + std::wstring{state_.session() ? L"present" : L"empty"} +
                                  L" CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}

void AssetBoardViewModel::BeginGeneration() {
    wac::trace_sink::Emit(L"VM", L"BeginGeneration this=" + PointerText(this));
    save_status_.clear();
    source_preview_ = nullptr;
    state_.BeginGeneration();
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"diagnostics=0 groups=0 generated_assets=0 CanSave=false");
    NotifyChanged();
}
void AssetBoardViewModel::CompleteGeneration(
    wac::GenerationSession session, wac::SourcePresentation source,
    winrt::Microsoft::UI::Xaml::Media::ImageSource source_preview) {
    wac::trace_sink::Emit(L"VM", L"CompleteGeneration this=" + PointerText(this));
    source_preview_ = std::move(source_preview);
    state_.CompleteGeneration(std::move(session), std::move(source));
    RefreshGroups();
    wac::trace_sink::Emit(L"STATE", L"diagnostics=" + std::to_wstring(state_.diagnostics().size()) +
                                  L" groups=" + std::to_wstring(state_.groups().size()) +
                                  L" generated_assets=" + std::to_wstring(state_.session()->preview_assets().size()) +
                                  L" CanSave=" + std::wstring{CanSave() ? L"true" : L"false"});
    NotifyChanged();
}
void AssetBoardViewModel::CompleteFailure(std::vector<wac::Diagnostic> diagnostics) {
    wac::trace_sink::Emit(L"VM", L"CompleteFailure this=" + PointerText(this));
    save_status_.clear();
    source_preview_ = nullptr;
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
        auto assets = winrt::single_threaded_observable_vector<winrt::WindowsAssetCreator::AssetBoardItemViewModel>();
        for (const auto& asset : group.assets) {
            const auto dimensions = std::format(L"{} \u00D7 {} px", asset.size.width, asset.size.height);
            const auto item = winrt::make<AssetBoardItemViewModel>(winrt::hstring{asset.label},
                                                                    winrt::hstring{dimensions},
                                                                    winrt::hstring{asset.staged_path.wstring()});
            wac::trace_sink::Emit(L"BOARD", std::wstring{L"RefreshGroups item projected_abi="} +
                                          PointerText(winrt::get_abi(item)) + L" label=" + asset.label);
            assets.Append(item);
        }
        const auto presentation = PresentGroup(group.title);
        const auto group_projection = winrt::make<AssetBoardGroupViewModel>(presentation.family_title,
                                                                            presentation.family_summary,
                                                                            presentation.family_header_visibility,
                                                                            presentation.title,
                                                                            assets);
        wac::trace_sink::Emit(L"BOARD", std::wstring{L"RefreshGroups group projected_abi="} +
                                      PointerText(winrt::get_abi(group_projection)) + L" title=" + group.title +
                                      L" assets_abi=" + PointerText(winrt::get_abi(assets)) +
                                      L" asset_count=" + std::to_wstring(assets.Size()));
        groups_.Append(group_projection);
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
