#include "AssetBoardState.h"

namespace wac {
void AssetBoardState::BeginGeneration() {
    session_.reset();
    diagnostics_.clear();
    phase_ = BoardPhase::processing;
}

void AssetBoardState::CompleteGeneration(GenerationSession session) {
    session_ = std::move(session);
    diagnostics_.clear();
    phase_ = BoardPhase::ready;
}

void AssetBoardState::CompleteFailure(std::vector<Diagnostic> diagnostics) {
    session_.reset();
    diagnostics_ = std::move(diagnostics);
    phase_ = BoardPhase::error;
}

BoardPhase AssetBoardState::phase() const noexcept { return phase_; }
bool AssetBoardState::can_save() const noexcept { return phase_ == BoardPhase::ready && session_.has_value(); }
std::optional<GenerationSession> const& AssetBoardState::session() const noexcept { return session_; }
std::span<Diagnostic const> AssetBoardState::diagnostics() const noexcept { return diagnostics_; }
}
