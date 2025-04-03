
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct TrackPiece {

        enum Type: u8 {
            Straight, Diagonal, CurveLeft, CurveRight, 
            Incline, CurveUp, CurveDown,
            StraightBallastless
        };

        enum Direction: u8 {
            Any, Ascending, Descending
        };

        struct TypeInfo {
            engine::Model::LoadArgs model;
            // must be applied to the model before rotation
            Mat<4> base_transform;
            // with 'base_transform' and rotation
            // (simply apply the built transform to get in world space)
            std::vector<Vec<3>> points;
            bool has_ballast;
            std::optional<Type> ballastless;
        };

        static const std::vector<TypeInfo>& types();

        u8 x, z;
        Type type;
        // to save memory, this variable represents the rotation of the piece
        // as the full number of quarter rotations, NOT radians or degrees
        // => multiply by (pi/2) to get the angle in radians
        i8 angle_q: 6;
        Direction direction: 2;
        i16 elevation;


        Mat<4> build_transform(
            u64 chunk_x, u64 chunk_z, u64 tiles_per_chunk, f64 units_per_tile
        ) const {
            const TrackPiece::TypeInfo& piece_info = TrackPiece::types()
                .at((size_t) this->type);
            f64 t_x = (f64) (chunk_x * tiles_per_chunk) + (f64) this->x + 0.5;
            f64 t_z = (f64) (chunk_z * tiles_per_chunk) + (f64) this->z + 0.5;
            Vec<3> pos = Vec<3>(t_x, 0, t_z) * units_per_tile
                + Vec<3>(0, this->elevation, 0);
            return Mat<4>::translate(pos)
                * Mat<4>::rotate_y((f64) this->angle_q * pi / 2.0)
                * piece_info.base_transform;
        }


        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& piece_type: TrackPiece::types()) {
                scene.load(piece_type.model);
            }
        }

    };

}