#include "StagingSession.h"
#include "ZipWriter.h"

namespace wac {
namespace {
void Cleanup(std::filesystem::path& root) noexcept {
    if (root.empty()) return;
    std::error_code error;
    std::filesystem::remove_all(root, error);
    root.clear();
}
}

GenerationSession::GenerationSession(std::filesystem::path root, StoreMsixProfile profile, std::vector<GeneratedAsset> assets)
    : staging_root_(std::move(root)), profile_(std::move(profile)), assets_(std::move(assets)) {}
GenerationSession::GenerationSession(GenerationSession&& other) noexcept
    : staging_root_(std::move(other.staging_root_)),
      profile_(std::move(other.profile_)),
      assets_(std::move(other.assets_)) {
    other.staging_root_.clear();
}
GenerationSession& GenerationSession::operator=(GenerationSession&& other) noexcept {
    if (this == &other) return *this;
    Cleanup(staging_root_);
    staging_root_ = std::move(other.staging_root_);
    profile_ = std::move(other.profile_);
    assets_ = std::move(other.assets_);
    other.staging_root_.clear();
    return *this;
}
GenerationSession::~GenerationSession() { Cleanup(staging_root_); }
std::filesystem::path const& GenerationSession::staging_root() const noexcept { return staging_root_; }
StoreMsixProfile const& GenerationSession::profile() const noexcept { return profile_; }
std::span<GeneratedAsset const> GenerationSession::preview_assets() const noexcept { return assets_; }
OperationResult GenerationSession::ExportZip(std::filesystem::path const& destination) const {
    std::vector<std::filesystem::path> entries;
    for (const auto& asset : profile_.png_assets()) entries.push_back(asset.relative_path);
    entries.push_back(profile_.ico_asset().relative_path);
    return WriteZip(staging_root_, entries, destination);
}
}
