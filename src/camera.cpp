
#include "camera.hpp"

namespace houseofatmos {
    
    void Camera::compute_matrices(int buff_width, int buff_height) {
        this->view = Mat<4>::look_at(this->pos, this->look_at, Camera::up);
        this->proj = Mat<4>::perspective(
            Camera::fov_deg * pi / 180.0, 
            buff_width, buff_height, 
            Camera::near, Camera::far
        );
        this->view_proj = this->proj * this->view;
    }

}