#pragma once
#ifndef CATA_SRC_OVERMAP_NOISE_H
#define CATA_SRC_OVERMAP_NOISE_H

#include "coordinates.h"
#include "game_constants.h"
#include "point.h"

namespace om_noise
{





class om_noise_layer
{
    public:




        virtual float noise_at( const point_om_omt &omt_local ) const = 0;
        virtual ~om_noise_layer() = default;
    protected:






        om_noise_layer( const point_abs_omt &global_base_point, const unsigned seed ) :
            om_global_base_point( global_base_point ),

            seed( seed % SIMPLEX_NOISE_RANDOM_SEED_LIMIT ) {
        }

        point_abs_omt global_omt_pos( const point_om_omt &local_omt_pos ) const {
            return om_global_base_point + local_omt_pos.raw();
        }

        float get_seed() const {
            return seed;
        }

    private:
        point_abs_omt om_global_base_point;
        float seed;
};

class om_noise_layer_forest : public om_noise_layer
{
    public:
        om_noise_layer_forest( const point_abs_omt &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point_om_omt &local_omt_pos ) const override;
};

class om_noise_layer_floodplain : public om_noise_layer
{
    public:
        om_noise_layer_floodplain( const point_abs_omt &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point_om_omt &local_omt_pos ) const override;
};

class om_noise_layer_lake : public om_noise_layer
{
    public:
        om_noise_layer_lake( const point_abs_omt &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point_om_omt &local_omt_pos ) const override;
};


class om_noise_layer_ocean : public om_noise_layer
{
    public:
        om_noise_layer_ocean( const point_abs_omt &global_base_point, unsigned seed )
            : om_noise_layer( global_base_point, seed ) {
        }

        float noise_at( const point_om_omt &local_omt_pos ) const override;
};

}

#endif
