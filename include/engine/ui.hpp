
#pragma once

#include "window.hpp"
#include "math.hpp"
#include <vector>
#include <optional>
#include <functional>

namespace houseofatmos::engine::ui {

    using namespace houseofatmos::engine;
    using namespace houseofatmos::engine::math;


    struct Element;

    struct ChildRef {
        const Element& element;
        size_t child_i;
    };

    using SizeFunc = Vec<2> (*)(
        const Element& self, const Element* parent,
        const Window& window, f64 unit
    );

    using PosFunc = Vec<2> (*)(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    );

    enum struct Direction {
        Horizontal, Vertical
    };

    struct Background {
        Texture::LoadArgs texture;
        Vec<2> offset;
        Vec<2> corner_size;
        Vec<2> edge_size;
    };

    struct Font {
        Texture::LoadArgs texture;
        Vec<2> offset;
        f64 height;
        f64 char_padding;
        std::string chars;
        std::vector<f64> char_widths;
        std::vector<f64> char_offsets_x;

        void compute_char_offsets() {
            this->char_offsets_x.reserve(this->char_widths.size());
            f64 offset = 0;
            for(f64 char_width: this->char_widths) {
                this->char_offsets_x.push_back(offset);
                offset += char_width + char_padding;
            }
        }
    };


    struct Manager;

    struct Element {

        private:
        Vec<2> size_px;
        Vec<2> position_px;

        bool hovering;


        public:
        Vec<2> size;
        SizeFunc size_func;
        
        Vec<2> position;
        PosFunc pos_func;

        std::vector<Element> children;
        Direction list_direction;

        const Background* background;
        const Background* background_hover;
        const Texture* texture;

        std::function<void()> on_click;

        std::string text;
        const Font* font;
        bool wrap_text;

        bool hidden;


        public:
        Element();
        Element& with_size(f64 width, f64 height, SizeFunc size_func) {
            this->size = Vec<2>(width, height);
            this->size_func = size_func;
            return *this;
        }
        Element& with_pos(f64 x, f64 y, PosFunc pos_func) {
            this->position = Vec<2>(x, y);
            this->pos_func = pos_func;
            return *this;
        }
        Element& with_children(
            std::vector<Element>&& children, Direction dir = Direction::Vertical
        ) {
            this->children = std::move(children);
            this->list_direction = dir;
            return *this;
        }
        Element& with_background(
            const Background* background,
            const Background* background_hover = nullptr
        ) {
            this->background = background;
            this->background_hover = background_hover;
            return *this;
        }
        Element& with_texture(const Texture* texture) {
            this->texture = texture;
            return *this;
        }
        Element& with_click_handler(std::function<void()> handler) {
            this->on_click = std::move(handler);
            return *this;
        }
        Element& with_text(
            std::string text, const Font* font, bool wrap_text = true
        ) {
            this->text = std::move(text);
            this->font = font;
            this->wrap_text = wrap_text;
            return *this;
        }

        bool is_hovered_over() const { return this->hovering; }
        const Vec<2>& final_size() const { return this->size_px; }
        const Vec<2>& final_pos() const { return this->position_px; }

        Vec<2> offset_of_child(size_t child_i) const;
        void update_root(const Window& window, f64 unit);
        void render_root(Manager& manager, Scene& scene, f64 unit) const;

        Element& with_padding(f64 amount);


        private:
        void update_size(const Element* parent, const Window& window, f64 unit);
        void update_position(
            std::optional<ChildRef> parent, const Window& window, f64 unit
        );
        void update_input(const Window& window);

        void render_background(Manager& manager, Scene& scene, f64 unit) const;
        void render_text(Manager& manager, Scene& scene, f64 unit) const;

    };


    namespace position {
        Vec<2> window_tl_units(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> window_tr_units(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> window_bl_units(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> window_br_units(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> window_fract(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> window_ndc(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> parent_units(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
        Vec<2> parent_fract(const Element& self, std::optional<ChildRef> parent, const Window& window, f64 unit);
    }


    namespace size {
        Vec<2> units(const Element& self, const Element* parent, const Window& window, f64 unit);
        Vec<2> parent_fract(const Element& self, const Element* parent, const Window& window, f64 unit);
        Vec<2> window_fract(const Element& self, const Element* parent, const Window& window, f64 unit);
        Vec<2> units_with_children(const Element& self, const Element* parent, const Window& window, f64 unit);
    }


    struct Manager {

        static const inline std::vector<Mesh::Attrib> mesh_attribs = {
            (Mesh::Attrib) { Mesh::F32, 2 }
        };

        static const inline Shader::LoadArgs shader_args = {
            "res/shaders/ui_vert.glsl", "res/shaders/ui_frag.glsl"
        };

        static void load_shaders(Scene& scene) {
            scene.load(Shader::Loader(Manager::shader_args));
        }


        private:
        Texture target = Texture(100, 100);
        Mesh quad = Mesh(Manager::mesh_attribs);
        Shader* shader;

        public:
        Element root = Element()
            .with_pos(0, 0, ui::position::window_tl_units)
            .with_size(1.0, 1.0, ui::size::window_fract);
        f64 unit_fract_size; // fraction of window height

        Manager(f64 unit_fract_size);
        Manager& with_element(Element&& element) {
            this->root.children.push_back(std::move(element));
            return *this;
        }

        const engine::Texture& output() const { return this->target; }

        void update(const Window& window);

        void render(const Window& window, Scene& scene);

        struct Instance {
            Vec<2> src_pos, src_size;
            Vec<2> dest_pos, dest_size;
        };

        void blit_texture(
            const Texture& src, std::span<const Instance> instances
        );

    };

}