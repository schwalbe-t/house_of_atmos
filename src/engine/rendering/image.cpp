
#include <engine/rendering.hpp>
#include <stb/stb_image.h>
#include <algorithm>

namespace houseofatmos::engine {

    Image::Image(u64 width, u64 height, const u8* raw_data) {
        this->width_px = width;
        this->height_px = height;
        this->pixels.resize(width * height);
        if(raw_data != nullptr) {
            const Color* data = (const Color*) raw_data;
            std::copy(data, data + this->pixels.size(), this->pixels.begin());
        } else {
            this->clear(Color(0, 0, 0));
        }
    }

    Image Image::from_resource(const LoadArgs& args) {
        std::vector<char> bytes = GenericResource::read_bytes(args.path);
        int width, height;
        stbi_uc* data = stbi_load_from_memory(
            (stbi_uc*) bytes.data(), bytes.size(), 
            &width, &height, nullptr, STBI_rgb_alpha
        );
        if(data == nullptr) {
            error("The file '" + args.path + "' contains invalid image data!");
        }
        auto result = Image(width, height, (const u8*) data);
        stbi_image_free((void*) data);
        return result;
    }

    void Image::clear(Color color) {
        this->pixels.assign(this->pixels.size(), color);
    }

    void Image::mirror_vertical() {
        auto swap = std::vector<Color>();
        swap.reserve(this->width());
        for(u64 y = 0; y < this->height() / 2; y += 1) {
            auto top_begin = this->pixels.begin() + y * this->width();
            auto bottom_begin = this->pixels.begin()
                + (this->height() - 1 - y) * this->width();
            std::copy(top_begin, top_begin + this->width(), swap.begin());
            std::copy(bottom_begin, bottom_begin + this->width(), top_begin);
            std::copy(swap.begin(), swap.begin() + this->width(), bottom_begin);
        }
    }

    void Image::mirror_horizontal() {
        for(u64 y = 0; y < this->height(); y += 1) {
            auto start = this->pixels.begin() + y * this->width();
            std::reverse(start, start + this->width());
        }
    }

}

