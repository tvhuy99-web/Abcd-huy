#pragma once
#ifndef CATA_SRC_ATLAS_BAKE_PLAN_H
#define CATA_SRC_ATLAS_BAKE_PLAN_H

#if defined(TILES)

#include <cstdint>
#include <optional>
#include <string>

class tileset;
namespace cata_shader
{
enum class memory_preset : int;
}




struct atlas_bake_plan {
    bool normal = true;
    bool shadow = true;
    bool night = true;
    bool overexposed = true;
    bool memory = true;
    bool silhouette = true;

    bool all_baked() const {
        return normal && shadow && night && overexposed && memory && silhouette;
    }




    bool needs_rebake_for( bool shader_variants_available,
                           std::optional<cata_shader::memory_preset> preset ) const {
        const bool skipped_variants = !shadow || !night || !overexposed || !silhouette;
        if( skipped_variants && !shader_variants_available ) {
            return true;
        }
        return !memory && ( !shader_variants_available || !preset.has_value() );
    }
};


atlas_bake_plan compute_atlas_bake_plan( bool shader_variants_available,
        std::optional<cata_shader::memory_preset> preset,
        bool tint_shader_available );


std::string bake_plan_summary( const atlas_bake_plan &plan );



enum class bundle_state {
    none,
    metadata_only,
    uploaded,
};
bundle_state classify_bundle( const tileset *ts );






bool bundle_needs_repair( const atlas_bake_plan &plan,
                          const std::string &mode_at_upload,
                          uint64_t fingerprint_at_upload,
                          const std::string &applied_mode,
                          uint64_t applied_fingerprint,
                          bool shader_variants_available );


enum class silhouette_miss {


    skipped_by_plan,

    normal_only_highlight,

    invalid,
};
silhouette_miss classify_silhouette_miss( const atlas_bake_plan &plan,
        std::optional<int> synthetic_highlight_index, int sprite_index );

#endif
#endif
