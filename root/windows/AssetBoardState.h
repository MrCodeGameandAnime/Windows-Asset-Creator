#pragma once

#include "../src/AssetGenerator.h"

namespace wac {
enum class BoardPhase { idle, processing, ready, saving, error };

struct BoardPreviewAsset {
    std::wstring label;
    PixelSize size;
    std::filesystem::path staged_path;
};

struct BoardPreviewGroup {
    std::wstring title;
    std::vector<BoardPreviewAsset> assets;
};

class AssetBoardState final {
public:
    void Reset();
    void BeginGeneration();
    void CompleteGeneration(GenerationSession session);
    void CompleteFailure(std::vector<Diagnostic> diagnostics);
    void BeginSave();
    void CompleteSaveCancelled();
    void CompleteSaveSuccess();
    void CompleteSaveFailure(Diagnostic diagnostic);

    BoardPhase phase() const noexcept;
    bool can_save() const noexcept;
    std::optional<GenerationSession> const& session() const noexcept;
    std::span<Diagnostic const> diagnostics() const noexcept;
    std::span<BoardPreviewGroup const> groups() const noexcept;
    bool can_reset() const noexcept;

private:
    BoardPhase phase_{BoardPhase::idle};
    std::optional<GenerationSession> session_;
    std::vector<Diagnostic> diagnostics_;
    std::vector<BoardPreviewGroup> groups_;
};
}
