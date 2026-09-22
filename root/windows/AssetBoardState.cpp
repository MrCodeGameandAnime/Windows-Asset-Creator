#include "AssetBoardState.h"
#include "../src/Diagnostics/TraceSink.h"

namespace wac {
namespace {
const wchar_t* PhaseName(BoardPhase phase) noexcept {
    switch (phase) {
    case BoardPhase::idle: return L"idle";
    case BoardPhase::processing: return L"processing";
    case BoardPhase::ready: return L"ready";
    case BoardPhase::saving: return L"saving";
    case BoardPhase::error: return L"error";
    }
    return L"unknown";
}

void TraceTransition(BoardPhase previous, BoardPhase next) noexcept {
    trace_sink::Emit(L"STATE", std::wstring{L"BoardPhase "} + PhaseName(previous) + L" -> " + PhaseName(next));
}
}

void AssetBoardState::Reset() {
    const auto previous = phase_;
    session_.reset();
    source_.reset();
    diagnostics_.clear();
    groups_.clear();
    phase_ = BoardPhase::idle;
    TraceTransition(previous, phase_);
}

void AssetBoardState::BeginGeneration() {
    const auto previous = phase_;
    session_.reset();
    source_.reset();
    diagnostics_.clear();
    groups_.clear();
    phase_ = BoardPhase::processing;
    TraceTransition(previous, phase_);
}

void AssetBoardState::CompleteGeneration(GenerationSession session, SourcePresentation source) {
    const auto previous = phase_;
    session_ = std::move(session);
    source_ = std::move(source);
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
    TraceTransition(previous, phase_);
}

void AssetBoardState::CompleteFailure(std::vector<Diagnostic> diagnostics) {
    const auto previous = phase_;
    session_.reset();
    source_.reset();
    diagnostics_ = std::move(diagnostics);
    groups_.clear();
    phase_ = BoardPhase::error;
    TraceTransition(previous, phase_);
}

void AssetBoardState::BeginSave() {
    if (phase_ == BoardPhase::ready && session_) {
        const auto previous = phase_;
        phase_ = BoardPhase::saving;
        TraceTransition(previous, phase_);
    }
}

void AssetBoardState::CompleteSaveCancelled() {
    if (phase_ == BoardPhase::saving && session_) {
        const auto previous = phase_;
        phase_ = BoardPhase::ready;
        TraceTransition(previous, phase_);
    }
}

void AssetBoardState::CompleteSaveSuccess() {
    if (phase_ == BoardPhase::saving && session_) {
        const auto previous = phase_;
        phase_ = BoardPhase::ready;
        TraceTransition(previous, phase_);
    }
}

void AssetBoardState::CompleteSaveFailure(Diagnostic diagnostic) {
    if (phase_ == BoardPhase::saving && session_) {
        const auto previous = phase_;
        diagnostics_.push_back(std::move(diagnostic));
        phase_ = BoardPhase::ready;
        TraceTransition(previous, phase_);
    }
}

BoardPhase AssetBoardState::phase() const noexcept { return phase_; }
bool AssetBoardState::can_save() const noexcept { return phase_ == BoardPhase::ready && session_.has_value(); }
bool AssetBoardState::can_reset() const noexcept {
    return phase_ == BoardPhase::ready || phase_ == BoardPhase::error;
}
std::optional<GenerationSession> const& AssetBoardState::session() const noexcept { return session_; }
std::optional<SourcePresentation> const& AssetBoardState::source() const noexcept { return source_; }
std::span<Diagnostic const> AssetBoardState::diagnostics() const noexcept { return diagnostics_; }
std::span<BoardPreviewGroup const> AssetBoardState::groups() const noexcept { return groups_; }
}
