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

private:
    winrt::hstring label_;
    winrt::hstring dimensions_;
    winrt::hstring preview_path_;
};

struct AssetBoardGroupViewModel : AssetBoardGroupViewModelT<AssetBoardGroupViewModel> {
    AssetBoardGroupViewModel() = default;
    AssetBoardGroupViewModel(winrt::hstring title,
                             winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets);

    winrt::hstring Title() const;
    winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardItemViewModel> Assets() const;

private:
    winrt::hstring title_;
    winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardItemViewModel> assets_{nullptr};
};

struct AssetBoardViewModel : AssetBoardViewModelT<AssetBoardViewModel> {
    AssetBoardViewModel();

    winrt::hstring SourceName() const;
    winrt::hstring SourceDimensions() const;
    winrt::hstring SourceFramingNote() const;
    winrt::hstring ValidationText() const;
    winrt::hstring ErrorText() const;
    bool IsIdle() const noexcept;
    bool IsBusy() const noexcept;
    bool HasError() const noexcept;
    bool CanSave() const noexcept;
    winrt::Microsoft::UI::Xaml::Visibility EmptyDropTargetVisibility() const noexcept;
    winrt::Windows::Foundation::Collections::IVectorView<winrt::WindowsAssetCreator::AssetBoardGroupViewModel> Groups() const;

    void BeginGeneration();
    void CompleteGeneration(wac::GenerationSession session);
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
