#pragma once

#include "AssetBoardState.h"
#include "pch.h"

namespace winrt::WindowsAssetCreator::implementation {
struct AssetBoardViewModel :
    winrt::implements<AssetBoardViewModel, winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged> {
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
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> Groups() const noexcept;

    void BeginGeneration();
    void CompleteGeneration(wac::GenerationSession session);
    void CompleteFailure(std::vector<wac::Diagnostic> diagnostics);

    winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
    void PropertyChanged(winrt::event_token const& token) noexcept;

private:
    void NotifyChanged();

    wac::AssetBoardState state_;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> groups_;
    winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> property_changed_;
};
}
