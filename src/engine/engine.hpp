
#include "raylib.h"

namespace houseofatmos::engine::rendering {

    struct FrameBuffer {
        int width;
        int height;
        Color* data;

        FrameBuffer(int width, int height);
        FrameBuffer(const FrameBuffer&) = delete;
        ~FrameBuffer();
        Color get_pixel_at(int x, int y);
        void set_pixel_at(int x, int y, Color c);
        void resize(int new_width, int new_height);
    };

}

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps);
    bool is_running();
    void display_buffer(rendering::FrameBuffer* buffer);
    void stop();

}