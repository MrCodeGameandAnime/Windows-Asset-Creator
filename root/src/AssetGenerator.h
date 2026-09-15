#pragma once
#include "Output/StagingSession.h"
namespace wac { class AssetGenerator final { public: GenerationResult<GenerationSession> Generate(std::filesystem::path const& source) const; }; }
