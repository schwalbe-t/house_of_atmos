
#pragma once

#include <druck/math.hpp>


namespace houseofatmos {

    using namespace druck::math;


    struct Camera {
        Vec<3> pos;
        Vec<3> look_at;
        static inline const Vec<3> up = Vec<3>(0.0, 1.0, 0.0);

        static inline const double fov_deg = 90.0;
        static inline const double near = 0.0001;
        static inline const double far = 1000000;

        Mat<4> view;
        Mat<4> proj;
        Mat<4> view_proj;

        void compute_matrices(int buff_width, int buff_height);
    };

}