
#include "train_tracks.hpp"

namespace houseofatmos::world {

    static std::vector<TrackPiece::TypeInfo> track_piece_type_infos = {
        /* Straight */ {
            engine::Model::LoadArgs(
                "res/trains/track_straight.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(0.0, 0.0, -2.5),
                Vec<3>(0.0, 0.0,  2.5)
            },
            true, // has ballast
            TrackPiece::StraightBallastless // ballastless variant
        },
        /* Diagonal */ {
            engine::Model::LoadArgs(
                "res/trains/track_diagonal.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(-2.5, 0.0, -2.5),
                Vec<3>( 2.5, 0.0,  2.5)
            },
            true, // has ballast
            std::nullopt
        },
        /* CurveLeft */ {
            engine::Model::LoadArgs(
                "res/trains/track_curve.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>::scale(Vec<3>(-1.0, 1.0, 1.0)), // flip along X axis
            {
                Vec<3>(2.5, 0.0, -2.5),
                Vec<3>(0.5, 0.0,  0.0),
                Vec<3>(0.0, 0.0,  2.5)
            },
            true, // has ballast
            std::nullopt
        },
        /* CurveRight */ {
            engine::Model::LoadArgs(
                "res/trains/track_curve.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(2.5, 0.0, -2.5),
                Vec<3>(0.5, 0.0,  0.0),
                Vec<3>(0.0, 0.0,  2.5)
            },
            true, // has ballast
            std::nullopt
        },
        /* Incline */ {
            engine::Model::LoadArgs(
                "res/trains/track_incline.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(0.0,  0.5, -2.5),
                Vec<3>(0.0, -0.5,  2.5)
            },
            true, // has ballast
            std::nullopt
        },
        /* CurveUp */ {
            engine::Model::LoadArgs(
                "res/trains/track_incline_bottom.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(0.0, 0.50, -2.5),
                Vec<3>(0.0, 0.05,  0.0),
                Vec<3>(0.0, 0.00,  2.5)
            },
            true, // has ballast
            std::nullopt
        },
        /* CurveDown */ {
            engine::Model::LoadArgs(
                "res/trains/track_incline_top.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(0.0,  0.00, -2.5),
                Vec<3>(0.0, -0.05,  0.0),
                Vec<3>(0.0, -0.50,  2.5)
            },
            true, // has ballast
            std::nullopt
        },

        /* StraightBallastless */ {
            engine::Model::LoadArgs(
                "res/trains/track_straight_ballastless.glb", 
                Renderer::model_attribs, engine::FaceCulling::Disabled
            ),
            Mat<4>(),
            {
                Vec<3>(0.0, 0.0, -2.5),
                Vec<3>(0.0, 0.0,  2.5)
            },
            false, // no ballast
            std::nullopt
        }
    };

    const std::vector<TrackPiece::TypeInfo>& TrackPiece::types() {
        return track_piece_type_infos;
    }

}