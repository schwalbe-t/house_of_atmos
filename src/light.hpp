
#pragma once

#include <engine/math.hpp>

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct DirectionalLight {

        Vec<3> position;
        Vec<3> focus_point;
        f64 radius;
        f64 near_plane = 0.01;
        f64 far_plane = 1.5;

        private:
        DirectionalLight(Vec<3> position, Vec<3> focus_point, f64 radius) {
            this->position = position;
            this->focus_point = focus_point;
            this->radius = radius;
        }

        public:
        static DirectionalLight from_to(
            Vec<3> position, Vec<3> focus_point, f64 radius
        ) {
            return DirectionalLight(position, focus_point, radius);
        }

        static DirectionalLight in_direction_to(
            Vec<3> direction, Vec<3> focus_point, f64 radius,
            f64 distance
        ) {
            Vec<3> position = focus_point - (direction.normalized() * distance);
            return DirectionalLight::from_to(position, focus_point, radius);
        }

        static DirectionalLight in_direction_from(
            Vec<3> direction, Vec<3> position, f64 radius,
            f64 distance
        ) {
            Vec<3> focus_point = position + (direction.normalized() * distance);
            return DirectionalLight::from_to(position, focus_point, radius);
        }

        Mat<4> compute_view_matrix() const {
            return Mat<4>::look_at(
                position, this->focus_point, Vec<3>(0, 1, 0)
            );
        }

        Mat<4> compute_proj_matrix() const {
            f64 distance = (this->position - this->focus_point).len();
            return Mat<4>::orthographic(
                -this->radius, this->radius, this->radius, -this->radius,
                distance * this->near_plane, 
                distance * this->far_plane
            );
        }

        Mat<4> compute_view_proj() const {
            return this->compute_proj_matrix() * this->compute_view_matrix();
        }
    };

}