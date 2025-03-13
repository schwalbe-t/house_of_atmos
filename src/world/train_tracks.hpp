
#pragma once

#include "agents.hpp"

namespace houseofatmos::world {

    struct TrackPiece {

        struct TypeInfo {
            engine::Model::LoadArgs model;
            // must be applied to the model before rotation
            Mat<4> base_transform;
            // WITHOUT 'base_transform', but with rotation
            std::vector<Vec<3>> points;
        };

        static const std::vector<TypeInfo>& types();

        enum Type: u8 {
            Straight, Diagonal, CurveLeft, CurveRight, 
            Incline, CurveUp, CurveDown
        };

        u8 x, z;
        Type type;
        // to save memory, this variable represents the rotation of the piece
        // as the full number of quarter rotations, NOT radians or degrees
        // => multiply by (pi/2) to get the angle in radians
        i8 rotation_quarters;
        i16 elevation;


        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& piece_type: TrackPiece::types()) {
                scene.load(piece_type.model);
            }
        }

    };

}