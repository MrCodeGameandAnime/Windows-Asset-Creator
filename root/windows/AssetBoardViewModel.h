#pragma once

#include "AssetBoardState.h"
#include "pch.h"
#include "AssetBoardItemViewModel.g.h"
#include "AssetBoardGroupViewModel.g.h"
#include "AssetBoardViewModel.g.h"

namespace winrt::WindowsAssetCreator::implementation {
struct AssetBoardItemViewModel : AssetBoardItemViewModelT<AssetBoardItemViewModel> {
    AssetBoardItemViewModel() = default;
    AssetBoardItemViewModel(winrt::hstring label, winrt::hstring dimensions, winrt::hstring preview_path);

    winrt::hstring Label() const;
    winrt::hstring Dimensions() const;
    winrt::hstring PreviewPath() const;
    winrt::Microsoft::UI::Xaml::Media::ImageSource Thumbnail() const;

private:
    winrt::hstring label_;
    winrt::hstring dimensions_;
    winrt::hstring preview_path_;
    winrt::Microsoft::UI::Xaml::Media::ImageSource thumbnail_{nullptr};
};

struct AssetBoardGroupViewModel : AssetBoardGroupViewModelT<AssetBoardGroupViewModel> {
    AssetBoardGroupViewModel() = default;
    AssetBoardGroupViewModel(winrt::hstring family_title,
                             winrt::hstring family_summary,
                             winrt::Microsoft::UI::Xaml::Visibility family_header_visibility,
                             winrt::hstring title,
                             winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets);

    winrt::hstring FamilyTitle() const;
    winrt::hstring FamilySummary() const;
    winrt::Microsoft::UI::Xaml::Visibility FamilyHeaderVisibility() const noexcept;
    winrt::hstring Title() const;
    winrt::hstring AssetCountText() const;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardItemViewModel> Assets() const;

private:
    winrt::hstring family_title_;
    winrt::hstring family_summary_;
    winrt::Microsoft::UI::Xaml::Visibility family_header_visibility_{winrt::Microsoft::UI::Xaml::Visibility::Collapsed};
    winrt::hstring title_;
    winrt::hstring asset_count_text_;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets_{nullptr};
};

struct AssetBoardViewModel : AssetBoardViewModelT<AssetBoardViewModel> {
    AssetBoardViewModel();

    winrt::hstring SourceName() const;
    winrt::hstring SourceDimensions() const;
    winrt::Microsoft::UI::Xaml::Media::ImageSource SourcePreview() const;
    winrt::Microsoft::UI::Xaml::Visibility SourceSummaryVisibility() const noexcept;
    winrt::hstring SourceFramingNote() const;
    winrt::hstring ValidationText() const;
    winrt::hstring ValidationDetail() const;
    bool HasValidationSuccess() const noexcept;
    winrt::hstring ErrorText() const;
    bool IsIdle() const noexcept;
    bool IsBusy() const noexcept;
    bool HasError() const noexcept;
    bool CanSave() const noexcept;
    bool CanReset() const noexcept;
    winrt::Microsoft::UI::Xaml::Visibility EmptyDropTargetVisibility() const noexcept;
    winrt::Microsoft::UI::Xaml::Visibility ReplacementDropTargetVisibility() const noexcept;
    winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardGroupViewModel> Groups() const;

    void Reset();
    void BeginGeneration();
    void CompleteGeneration(wac::GenerationSession session, wac::SourcePresentation source,
                            winrt::Microsoft::UI::Xaml::Media::ImageSource source_preview);
    void CompleteFailure(std::vector<wac::Diagnostic> diagnostics);
    bool BeginSave();
    wac::OperationResult ExportZip(std::filesystem::path const& destination) const;
    void CompleteSaveCancelled();
    void CompleteSaveSuccess(std::filesystem::path const& destination, bool explorer_opened);
    void CompleteSaveFailure(wac::Diagnostic diagnostic);
    winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
    void PropertyChanged(winrt::event_token const& token) noexcept;

private:
    void RefreshGroups();
    void NotifyChanged();

    wac::AssetBoardState state_;
    winrt::Microsoft::UI::Xaml::Media::ImageSource source_preview_{nullptr};
    winrt::hstring save_status_;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::WindowsAssetCreator::AssetBoardGroupViewModel> groups_;
    winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> property_changed_;
};
}

namespace winrt::WindowsAssetCreator::factory_implementation {
struct AssetBoardItemViewModel : AssetBoardItemViewModelT<AssetBoardItemViewModel, implementation::AssetBoardItemViewModel> {};
struct AssetBoardGroupViewModel : AssetBoardGroupViewModelT<AssetBoardGroupViewModel, implementation::AssetBoardGroupViewModel> {};
struct AssetBoardViewModel : AssetBoardViewModelT<AssetBoardViewModel, implementation::AssetBoardViewModel> {};
}
