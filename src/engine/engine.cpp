
#include "engine.hpp"
#include <cstdlib>
#include <utility>

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        SetTraceLogLevel(LOG_ERROR);
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
        bool buffer_size_correct = buffer->width == GetScreenWidth()
            && buffer->height == GetScreenHeight();
        if(buffer_size_correct) { return; }
        auto new_buffer = rendering::FrameBuffer(
            GetScreenWidth(), GetScreenHeight()
        );
        *buffer = std::move(new_buffer);
    }

    void stop() {
        CloseWindow();
        std::exit(1);
    }

}