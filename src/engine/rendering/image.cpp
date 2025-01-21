
#include <engine/rendering.hpp>
#include <stb/stb_image.h>

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

}

