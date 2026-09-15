#pragma once

#include "../src/AssetGenerator.h"

namespace wac {
enum class BoardPhase { idle, processing, ready, saving, error };

class AssetBoardState final {
public:
    void BeginGeneration();
    void CompleteGeneration(GenerationSession session);
    void CompleteFailure(std::vector<Diagnostic> diagnostics);

    BoardPhase phase() const noexcept;
    bool can_save() const noexcept;
    std::optional<GenerationSession> const& session() const noexcept;
    std::span<Diagnostic const> diagnostics() const noexcept;

private:
    BoardPhase phase_{BoardPhase::idle};
    std::optional<GenerationSession> session_;
    std::vector<Diagnostic> diagnostics_;
};
}
