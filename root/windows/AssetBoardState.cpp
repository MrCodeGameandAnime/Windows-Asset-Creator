#include "AssetBoardState.h"

namespace wac {
void AssetBoardState::BeginGeneration() {
    session_.reset();
    diagnostics_.clear();
    groups_.clear();
    phase_ = BoardPhase::processing;
}

void AssetBoardState::CompleteGeneration(GenerationSession session) {
    session_ = std::move(session);
    diagnostics_.clear();
    groups_.clear();
    for (const auto& preview : session_->preview_assets()) {
        auto group = std::find_if(groups_.begin(), groups_.end(), [&preview](const BoardPreviewGroup& candidate) {
            return candidate.title == preview.spec.group;
        });
        if (group == groups_.end()) {
            groups_.push_back({preview.spec.group, {}});
            group = std::prev(groups_.end());
        }
        group->assets.push_back({preview.spec.relative_path.filename().wstring(), preview.spec.size, preview.staged_path});
    }
    phase_ = BoardPhase::ready;
}

void AssetBoardState::CompleteFailure(std::vector<Diagnostic> diagnostics) {
    session_.reset();
    diagnostics_ = std::move(diagnostics);
    groups_.clear();
    phase_ = BoardPhase::error;
}

void AssetBoardState::BeginSave() {
    if (phase_ == BoardPhase::ready && session_) phase_ = BoardPhase::saving;
}

void AssetBoardState::CompleteSaveCancelled() {
    if (phase_ == BoardPhase::saving && session_) phase_ = BoardPhase::ready;
}

void AssetBoardState::CompleteSaveSuccess() {
    if (phase_ == BoardPhase::saving && session_) phase_ = BoardPhase::ready;
}

void AssetBoardState::CompleteSaveFailure(Diagnostic diagnostic) {
    if (phase_ == BoardPhase::saving && session_) {
        diagnostics_.push_back(std::move(diagnostic));
        phase_ = BoardPhase::ready;
    }
}

BoardPhase AssetBoardState::phase() const noexcept { return phase_; }
bool AssetBoardState::can_save() const noexcept { return phase_ == BoardPhase::ready && session_.has_value(); }
std::optional<GenerationSession> const& AssetBoardState::session() const noexcept { return session_; }
std::span<Diagnostic const> AssetBoardState::diagnostics() const noexcept { return diagnostics_; }
std::span<BoardPreviewGroup const> AssetBoardState::groups() const noexcept { return groups_; }
}
