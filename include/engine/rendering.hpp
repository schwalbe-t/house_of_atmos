
#pragma once

#include "math.hpp"
#include "scene.hpp"
#include <vector>
#include <span>
#include <unordered_map>
#include <utility>

namespace houseofatmos::engine {

    struct Texture {
        struct LoadArgs {
            std::string path;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const {
                return "Texture@'" + path + "'"; 
            }
        };
        using Loader = Resource<Texture, LoadArgs>;

        private:
        i64 width_px;
        i64 height_px;

        u64 fbo_id;
        u64 tex_id;
        u64 dbo_id;
        bool moved;

        Texture(i64 width, i64 height, const void* data);


        public:
        Texture(i64 width, i64 height);
        static Texture from_resource(const LoadArgs& arg);
        Texture(const Texture& other) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(const Texture& other) = delete;
        Texture& operator=(Texture&& other) noexcept;
        ~Texture();

        i64 width() const;
        i64 height() const;

        u64 internal_fbo_id() const;
        u64 internal_tex_id() const;

        void clear_color(Vec<4> color) const;
        void clear_depth(f64 depth) const;

        void resize_fast(i64 width, i64 height);
        void resize(i64 width, i64 height);

        void blit(const Texture& dest, f64 x, f64 y, f64 w, f64 h) const;
        void internal_blit(
            u64 dest_fbo_id, u32 dest_width, u32 dest_height,
            f64 x, f64 y, f64 w, f64 h
        ) const;

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
 
        static u64 max_textures();

        void set_uniform(std::string_view name, const Texture& texture);

        void set_uniform(std::string_view name, f64 value);
        void set_uniform(std::string_view name, Vec<2> value);
        void set_uniform(std::string_view name, Vec<3> value);
        void set_uniform(std::string_view name, Vec<4> value);
        void set_uniform(std::string_view name, std::span<f64> value);
        void set_uniform(std::string_view name, std::span<Vec<2>> value);
        void set_uniform(std::string_view name, std::span<Vec<3>> value);
        void set_uniform(std::string_view name, std::span<Vec<4>> value);

        void set_uniform(std::string_view name, i64 value);
        void set_uniform(std::string_view name, IVec<2> value);
        void set_uniform(std::string_view name, IVec<3> value);
        void set_uniform(std::string_view name, IVec<4> value);
        void set_uniform(std::string_view name, std::span<i64> value);
        void set_uniform(std::string_view name, std::span<IVec<2>> value);
        void set_uniform(std::string_view name, std::span<IVec<3>> value);
        void set_uniform(std::string_view name, std::span<IVec<4>> value);

        void set_uniform(std::string_view name, u64 value);
        void set_uniform(std::string_view name, UVec<2> value);
        void set_uniform(std::string_view name, UVec<3> value);
        void set_uniform(std::string_view name, UVec<4> value);
        void set_uniform(std::string_view name, std::span<u64> value);
        void set_uniform(std::string_view name, std::span<UVec<2>> value);
        void set_uniform(std::string_view name, std::span<UVec<3>> value);
        void set_uniform(std::string_view name, std::span<UVec<4>> value);

        void set_uniform(std::string_view name, const Mat<2>& value);
        void set_uniform(std::string_view name, const Mat<3>& value);
        void set_uniform(std::string_view name, const Mat<4>& value);
        void set_uniform(std::string_view name, const Mat<2, 3>& value);
        void set_uniform(std::string_view name, const Mat<3, 2>& value);
        void set_uniform(std::string_view name, const Mat<2, 4>& value);
        void set_uniform(std::string_view name, const Mat<4, 2>& value);
        void set_uniform(std::string_view name, const Mat<3, 4>& value);
        void set_uniform(std::string_view name, const Mat<4, 3>& value);
        void set_uniform(std::string_view name, std::span<Mat<2>> value);
        void set_uniform(std::string_view name, std::span<Mat<3>> value);
        void set_uniform(std::string_view name, std::span<Mat<4>> value);
        void set_uniform(std::string_view name, std::span<Mat<2, 3>> value);
        void set_uniform(std::string_view name, std::span<Mat<3, 2>> value);
        void set_uniform(std::string_view name, std::span<Mat<2, 4>> value);
        void set_uniform(std::string_view name, std::span<Mat<4, 2>> value);
        void set_uniform(std::string_view name, std::span<Mat<3, 4>> value);
        void set_uniform(std::string_view name, std::span<Mat<4, 3>> value);

    };


    struct Mesh {

        private:
        std::vector<u8> attrib_sizes;
        u16 vertex_size;
        std::vector<f32> vertices;
        std::vector<u16> elements;
        
        u64 vbo_id;
        u64 ebo_id;
        u64 buff_index_count;
        bool modified;
        bool moved;

        void init_buffers();
        void bind_properties() const;
        void unbind_properties() const;
        void delete_buffers() const;


        public:
        Mesh(std::initializer_list<u8> attrib_sizes);
        Mesh(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

        u16 add_vertex(std::span<f32> data);
        u16 add_vertex(std::initializer_list<f32> data);
        u16 vertex_count() const;
        void add_element(u16 a, u16 b, u16 c);
        u32 element_count() const;
        void clear();

        void submit();
        void render(const Shader& shader, const Texture& dest);
        void internal_render(
            const Shader& shader, u64 dest_fbo_id, 
            i32 dest_width, i32 dest_height
        );

        bool was_moved() {
            return this->moved;
        }

    };


    struct Property {
        enum Type {
            Position,
            UvMapping,
            Normal
        };
        Type type;

        u8 size() {
            switch(this->type) {
                case Property::Position: return 3;
                case Property::UvMapping: return 2;
                case Property::Normal: return 3;
                default: return 0;
            }
        }
    };

    struct Model {
        struct LoadArgs {
            std::string path;
            std::vector<Property> properties;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const { 
                return "Model@'" + path + "'"; 
            }
        };
        using Loder = Resource<Model, LoadArgs>;

        private:
        // ... properties here ...

        Model(/* ... property values here ... */);

        public:
        Model(const std::string& path);
        static Model from_resource(const LoadArgs& args);
        Model(const Model& other) = delete;
        Model& operator=(const Model& other) = delete;

    };

}