
#include "engine.hpp"
#include <cstdlib>

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(width, height, title);
        SetTargetFPS(fps);
    }

    bool is_running() {
        return !WindowShouldClose();
    }

    void display_buffer(rendering::FrameBuffer* buffer) {
        Image img;
        img.data = buffer->data;
        img.width = buffer->width;
        img.height = buffer->height;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        img.mipmaps = 1;
        Texture2D texture = LoadTextureFromImage(img);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(texture, 0, 0, WHITE);
        EndDrawing();
        UnloadTexture(texture);
        buffer->resize(GetScreenWidth(), GetScreenHeight());
    }

    void stop() {
        CloseWindow();
        std::exit(1);
    }

}