
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


        public:
        Element();
        Element with_size(f64 width, f64 height, SizeFunc size_func) {
            this->size = Vec<2>(width, height);
            this->size_func = size_func;
            return *this;
        }
        Element with_pos(f64 x, f64 y, PosFunc pos_func) {
            this->position = Vec<2>(x, y);
            this->pos_func = pos_func;
            return *this;
        }
        Element with_children(
            std::vector<Element> children, Direction dir = Direction::Vertical
        ) {
            this->children = std::move(children);
            this->list_direction = dir;
            return *this;
        }
        Element with_background(
            const Background* background,
            const Background* background_hover = nullptr
        ) {
            this->background = background;
            this->background_hover = background_hover;
            return *this;
        }
        Element with_texture(const Texture* texture) {
            this->texture = texture;
            return *this;
        }
        Element with_click_handler(std::function<void()> handler) {
            this->on_click = std::move(handler);
            return *this;
        }
        Element with_text(std::string text) {
            this->text = std::move(text);
            return *this;
        }

        bool is_hovered_over() const { return this->hovering; }
        const Vec<2>& final_size() const { return this->size_px; }
        const Vec<2>& final_pos() const { return this->position_px; }

        Vec<2> offset_of_child(size_t child_i) const;
        void update_root(const Window& window, f64 unit);
        void render_root();


        private:
        void update_size(const Element* parent, const Window& window, f64 unit);
        void update_position(
            std::optional<ChildRef> parent, const Window& window, f64 unit
        );
        void update_input(const Window& window);

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
    }

}