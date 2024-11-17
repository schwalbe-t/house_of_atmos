
#include "engine.hpp"
#include "logging.hpp"
#include <cstdlib>
#include <utility>

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps) {
        SetTraceLogLevel(LOG_ERROR);
        SetTraceLogCallback(&houseofatmos::engine::logging::raylib);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(width, height, title);
        SetTargetFPS(fps);
    }

    bool is_running() {
        return !WindowShouldClose();
    }

    void display_buffer(rendering::Surface* buffer) {
        Image img;
        img.data = buffer->color;
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
        auto new_buffer = rendering::Surface(
            GetScreenWidth(), GetScreenHeight()
        );
        *buffer = std::move(new_buffer);
    }

    void stop() {
        CloseWindow();
        std::exit(1);
    }

}