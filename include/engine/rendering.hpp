
#pragma once

#include "math.hpp"
#include "scene.hpp"
#include <vector>
#include <span>
#include <unordered_map>
#include <utility>
#include <memory>

namespace houseofatmos::engine {

    using namespace math;


    struct Image {
        struct LoadArgs {
            std::string path;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const {
                return "Image@'" + path + "'"; 
            }
        };
        using Loader = Resource<Image, LoadArgs>;

        struct Color {
            u8 r, g, b, a; 

            Color(u8 r = 0, u8 g = 0, u8 b = 0, u8 a = 255) {
                this->r = r;
                this->g = g;
                this->b = b;
                this->a = a;
            }
        };

        private:
        u64 width_px;
        u64 height_px;
        std::vector<Color> pixels;

        public:
        Image(u64 width, u64 height, const u8* data = nullptr);
        static Image from_resource(const LoadArgs& args);

        u64 width() const { return this->width_px; }
        u64 height() const { return this->height_px; }

        Color& pixel_at(u64 x, u64 y) {
            if(x >= this->width_px || y >= this->height_px) {
                error("The pixel position (" 
                    + std::to_string(x) + ", " 
                    + std::to_string(y) + ") is out of bounds"
                    + "for an image of size "
                    + std::to_string(this->width_px) + "x"
                    + std::to_string(this->height_px) + "!"
                );
            }
            return this->pixels[y * this->width_px + x];
        }
        const Color& pixel_at(u64 x, u64 y) const {
            return ((Image*) this)->pixel_at(x, y);
        }

        const Color* data() const { return this->pixels.data(); }

        void clear(Color color);
        void mirror_vertical();
        void mirror_horizontal();

    };


    struct Shader;


    struct Texture {
        struct LoadArgs {
            std::string path;
            bool mirror_vertical = false;

            std::string identifier() const {
                return this->path + "|" + (this->mirror_vertical? "v" : "n"); 
            }
            std::string pretty_identifier() const {
                return std::string("Texture[") +
                    + (this->mirror_vertical? "vertical mirror" : "no mirror")
                    + "]@'" 
                    + this->path 
                    + "'"; 
            }
        };
        using Loader = Resource<Texture, LoadArgs>;

        private:
        u64 width_px;
        u64 height_px;

        u64 fbo_id;
        u64 tex_id;
        u64 dbo_id;
        bool moved;


        public:
        Texture(u64 width, u64 height, const u8* data);
        Texture(u64 width, u64 height);
        Texture(const Image& image);
        static Texture from_resource(const LoadArgs& arg);
        Texture(const Texture& other) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(const Texture& other) = delete;
        Texture& operator=(Texture&& other) noexcept;
        ~Texture();

        u64 width() const;
        u64 height() const;

        u64 internal_fbo_id() const;
        u64 internal_tex_id() const;

        void clear_color(Vec<4> color) const;
        void clear_depth(f64 depth) const;

        void resize_fast(u64 width, u64 height);
        void resize(u64 width, u64 height);

        void blit(const Texture& dest, f64 x, f64 y, f64 w, f64 h) const;
        void internal_blit(
            u64 dest_fbo_id, u32 dest_width, u32 dest_height,
            f64 x, f64 y, f64 w, f64 h
        ) const;
        void blit(const Texture& dest, Shader& shader) const;

    };


    struct Shader {
        struct LoadArgs {
            std::string vertex_path;
            std::string fragment_path;

            std::string identifier() const {
                return vertex_path + "|||" + fragment_path;
            }
            std::string pretty_identifier() const {
                return "Shader@'" + vertex_path + "'&'" + fragment_path + "'";
            }
        };
        using Loader = Resource<Shader, LoadArgs>;

        private:
        // <uniform name> -> <tex id>
        std::unordered_map<std::string, u64> uniform_textures;
        // <tex id> -> <count of uniforms using it>
        std::unordered_map<u64, u64> texture_uniform_count;
        // <tex id> -> <tex slot>
        std::unordered_map<u64, u64> texture_slots;
        // free tex slots
        std::vector<u64> free_tex_slots;
        u64 next_slot;

        u64 vert_id;
        u64 frag_id;
        u64 prog_id;
        bool moved;


        public:
        Shader(const std::string& vertex_src, const std::string& fragment_src);
        static Shader from_resource(const LoadArgs& args);
        Shader(const Shader& other) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(const Shader& other) = delete;
        Shader& operator=(Shader&& other) noexcept;
        ~Shader();

        void internal_bind() const;
        void internal_unbind() const;
 
        static size_t max_textures();

        void set_uniform(std::string_view name, const Texture& texture);

        void set_uniform(std::string_view name, f64 value);
        void set_uniform(std::string_view name, Vec<2> value);
        void set_uniform(std::string_view name, Vec<3> value);
        void set_uniform(std::string_view name, Vec<4> value);
        void set_uniform(std::string_view name, std::span<const f64> value);
        void set_uniform(std::string_view name, std::span<const Vec<2>> value);
        void set_uniform(std::string_view name, std::span<const Vec<3>> value);
        void set_uniform(std::string_view name, std::span<const Vec<4>> value);

        void set_uniform(std::string_view name, i64 value);
        void set_uniform(std::string_view name, IVec<2> value);
        void set_uniform(std::string_view name, IVec<3> value);
        void set_uniform(std::string_view name, IVec<4> value);
        void set_uniform(std::string_view name, std::span<const i64> value);
        void set_uniform(std::string_view name, std::span<const IVec<2>> value);
        void set_uniform(std::string_view name, std::span<const IVec<3>> value);
        void set_uniform(std::string_view name, std::span<const IVec<4>> value);

        void set_uniform(std::string_view name, u64 value);
        void set_uniform(std::string_view name, UVec<2> value);
        void set_uniform(std::string_view name, UVec<3> value);
        void set_uniform(std::string_view name, UVec<4> value);
        void set_uniform(std::string_view name, std::span<const u64> value);
        void set_uniform(std::string_view name, std::span<const UVec<2>> value);
        void set_uniform(std::string_view name, std::span<const UVec<3>> value);
        void set_uniform(std::string_view name, std::span<const UVec<4>> value);

        void set_uniform(std::string_view name, const Mat<2>& value);
        void set_uniform(std::string_view name, const Mat<3>& value);
        void set_uniform(std::string_view name, const Mat<4>& value);
        void set_uniform(std::string_view name, const Mat<2, 3>& value);
        void set_uniform(std::string_view name, const Mat<3, 2>& value);
        void set_uniform(std::string_view name, const Mat<2, 4>& value);
        void set_uniform(std::string_view name, const Mat<4, 2>& value);
        void set_uniform(std::string_view name, const Mat<3, 4>& value);
        void set_uniform(std::string_view name, const Mat<4, 3>& value);
        void set_uniform(std::string_view name, std::span<const Mat<2>> value);
        void set_uniform(std::string_view name, std::span<const Mat<3>> value);
        void set_uniform(std::string_view name, std::span<const Mat<4>> value);
        void set_uniform(std::string_view name, std::span<const Mat<2, 3>> value);
        void set_uniform(std::string_view name, std::span<const Mat<3, 2>> value);
        void set_uniform(std::string_view name, std::span<const Mat<2, 4>> value);
        void set_uniform(std::string_view name, std::span<const Mat<4, 2>> value);
        void set_uniform(std::string_view name, std::span<const Mat<3, 4>> value);
        void set_uniform(std::string_view name, std::span<const Mat<4, 3>> value);

    };


    struct Mesh {

        enum AttribType {
            F32,
            I8, I16, I32,
            U8, U16, U32
        };

        struct Attrib {
            AttribType type;
            size_t count;

            size_t type_size_bytes() const;
            size_t size_bytes() const;
            int gl_type_constant() const;
            std::string display_type() const;
            std::string display() const;
        };

        private:
        std::vector<Attrib> attributes;
        size_t vertex_size;
        std::vector<u8> vertex_data;
        size_t vertices;
        size_t current_attrib;
        std::vector<u16> elements;
        
        u64 vbo_id;
        u64 ebo_id;
        bool modified;
        bool moved;

        void init_buffers();
        void bind_properties() const;
        void unbind_properties() const;
        void delete_buffers() const;


        public:
        Mesh(std::span<const Attrib> attributes);
        Mesh(std::initializer_list<Attrib> attributes);
        Mesh(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

        static size_t max_attributes();

        void start_vertex();
        void put_f32(std::span<const f32> values);
        void put_f32(std::initializer_list<f32> values);
        void put_i8(std::span<const i8> values);
        void put_i8(std::initializer_list<i8> values);
        void put_i16(std::span<const i16> values);
        void put_i16(std::initializer_list<i16> values);
        void put_i32(std::span<const i32> values);
        void put_i32(std::initializer_list<i32> values);
        void put_u8(std::span<const u8> values);
        void put_u8(std::initializer_list<u8> values);
        void put_u16(std::span<const u16> values);
        void put_u16(std::initializer_list<u16> values);
        void put_u32(std::span<const u32> values);
        void put_u32(std::initializer_list<u32> values);
        void unsafe_put_raw(std::span<const u8> data);
        void unsafe_next_attr();
        u16 complete_vertex();

        u16 vertex_count() const;
        void add_element(u16 a, u16 b, u16 c);
        u32 element_count() const;
        void clear();

        void submit();
        void render(
            const Shader& shader, const Texture& dest,
            size_t count = 1, bool wireframe = false, bool depth_test = true
        );
        void internal_render(
            const Shader& shader, u64 dest_fbo_id, 
            i32 dest_width, i32 dest_height,
            size_t count,  bool wireframe = false, bool depth_test = true
        );

        bool was_moved() {
            return this->moved;
        }

    };

}