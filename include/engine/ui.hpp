
#pragma once

#include "window.hpp"
#include "math.hpp"
#include <list>
#include <vector>
#include <optional>
#include <functional>
#include <iterator>

namespace houseofatmos::engine::ui {

    using namespace houseofatmos::engine;
    using namespace houseofatmos::engine::math;


    struct Element;

    struct ChildRef {
        const Element& element;
        size_t child_i;
    };

    struct LayoutContext {
        Element* self;
        const Element* parent;
        size_t child_i;
        const Window* window;
        f64 unit;

        LayoutContext(Element& self, const Window& window, f64 unit): 
            self(&self), parent(nullptr), child_i(0), 
            window(&window), unit(unit) {}

        LayoutContext for_child(Element& child, size_t child_i) {
            auto ctx = LayoutContext(child, *this->window, this->unit);
            ctx.parent = this->self;
            ctx.child_i = child_i;
            return ctx;
        }
    };

    #define IMPL_LAYOUT_FUNC_OPERATOR(OP) \
        LayoutFunc operator OP(LayoutFunc other) const { \
            return LayoutFunc([l = this->impl, r = other.impl](auto ctx) { \
                return l(ctx) OP r(ctx); \
            }); \
        } \
        LayoutFunc operator OP(f64 other) const { \
            return LayoutFunc([l = this->impl, r = other](auto ctx) { \
                return l(ctx) OP r; \
            }); \
        }

    struct LayoutFunc {
        std::function<f64 (LayoutContext)> impl;

        IMPL_LAYOUT_FUNC_OPERATOR(+)
        IMPL_LAYOUT_FUNC_OPERATOR(-)
        IMPL_LAYOUT_FUNC_OPERATOR(*)
        IMPL_LAYOUT_FUNC_OPERATOR(/)

        LayoutFunc min(LayoutFunc other) const {
            return LayoutFunc([l = this->impl, r = other.impl](auto ctx) {
                return std::min(l(ctx), r(ctx));
            });
        }

        LayoutFunc max(LayoutFunc other) const {
            return LayoutFunc([l = this->impl, r = other.impl](auto ctx) {
                return std::max(l(ctx), r(ctx));
            });
        }
    };

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
        std::string_view chars;
        std::vector<f64> char_widths;
        std::optional<std::vector<f64>> char_offsets_x;

        Font(
            Texture::LoadArgs texture, Vec<2> offset, 
            f64 height, f64 char_padding,
            std::string_view chars, std::vector<f64> char_widths
        ): texture(texture), offset(offset), 
            height(height), char_padding(char_padding), 
            chars(chars), char_widths(char_widths), 
            char_offsets_x(std::nullopt) {}

        void compute_char_offsets() {
            this->char_offsets_x = std::vector<f64>();
            this->char_offsets_x->reserve(this->char_widths.size());
            f64 offset = 0;
            for(f64 char_width: this->char_widths) {
                this->char_offsets_x->push_back(offset);
                offset += char_width + char_padding;
            }
        }
    };


    extern LayoutFunc null;
    extern LayoutFunc unit;

    namespace width {
        extern LayoutFunc parent;
        extern LayoutFunc window;
        extern LayoutFunc children;
        extern LayoutFunc text;
    }

    namespace height {
        extern LayoutFunc parent;
        extern LayoutFunc window;
        extern LayoutFunc children;
        extern LayoutFunc text;
    }

    namespace horiz {
        LayoutFunc in_window_fract(f64 fract);
        LayoutFunc in_parent_fract(f64 fract);
        LayoutFunc window_ndc(f64 ndc);
        extern LayoutFunc parent;
        extern LayoutFunc list;
        extern LayoutFunc width;
    }

    namespace vert {
        LayoutFunc in_window_fract(f64 fract);
        LayoutFunc in_parent_fract(f64 fract);
        LayoutFunc window_ndc(f64 ndc);
        extern LayoutFunc parent;
        extern LayoutFunc list;
        extern LayoutFunc height;
    }


    static inline const bool include_dragging = true;
    static inline const bool ignore_dragging = false;

    struct Manager;

    struct Element {

        private:
        Vec<2> size_px;
        Vec<2> position_px;

        bool hovering;


        public:
        LayoutFunc width_func;
        LayoutFunc height_func;
        
        LayoutFunc horiz_func;
        LayoutFunc vert_func;

        // 'std::list' because:
        //   - preserves the addresses of children even on push / pop
        //     (this is the most important reason)
        //   - push / pop are constant time (very very common operation)
        //   - iteration is still constant time
        //   - direct indexing is usually a low index, such as [0], [1] or [2]
        std::list<Element> children;
        Direction list_direction;

        const Background* background;
        const Background* background_hover;
        const Texture* texture;

        std::function<void (ui::Element&, Vec<2>)> on_click;
        bool handle_dragging;

        std::string text;
        const Font* font;
        bool wrap_text;

        bool hidden;
        bool phantom;

        Element** handle;


        public:
        Element();
        Element(Element&& other) noexcept;
        Element& operator=(Element&& other) noexcept;
        ~Element();
        Element& with_size(LayoutFunc width_func, LayoutFunc height_func) {
            this->width_func = width_func;
            this->height_func = height_func;
            return *this;
        }
        Element& with_pos(LayoutFunc horiz_func, LayoutFunc vert_func) {
            this->horiz_func = horiz_func;
            this->vert_func = vert_func;
            return *this;
        }
        Element& with_list_dir(Direction list_dir) {
            this->list_direction = list_dir;
            return *this;
        }
        Element& with_child(Element&& child) {
            this->children.push_back(std::move(child));
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
        Element& with_click_handler(
            std::function<void ()>&& handler, 
            bool handle_dragging = ui::ignore_dragging
        ) {
            this->on_click = [h = std::move(handler)](const ui::Element& e, Vec<2> c) {
                (void) e; (void) c;
                h();    
            };
            this->handle_dragging = handle_dragging;
            return this->as_phantom(false);
        }
        Element& with_click_handler(
            std::function<void (ui::Element&, Vec<2>)>&& handler, 
            bool handle_dragging = ui::ignore_dragging
        ) {
            this->on_click = std::move(handler);
            this->handle_dragging = handle_dragging;
            return this->as_phantom(false);
        }
        Element& with_text(
            std::string text, const Font* font, bool wrap_text = true
        ) {
            this->text = std::move(text);
            this->font = font;
            this->wrap_text = wrap_text;
            return *this;
        }
        Element& as_hidden(bool is_hidden) {
            this->hidden = is_hidden;
            return *this;
        }
        Element& as_phantom(bool phantom = true) {
            this->phantom = phantom;
            return *this;
        }
        Element& with_handle(Element** handle_ptr) {
            this->handle = handle_ptr;
            if(this->handle != nullptr) {
                *this->handle = this;
            }
            return *this;
        }
        Element&& as_movable() { return std::move(*this); }

        bool is_hovered_over() const { return this->hovering; }
        const Vec<2>& final_size() const { return this->size_px; }
        const Vec<2>& final_pos() const { return this->position_px; }

        // these use a constexpr index so noone gets any idea of using these
        // in a loop >:/
        template<size_t N>
        ui::Element& child_at() {
            return *std::next(this->children.begin(), N);
        }
        template<size_t N>
        const ui::Element& child_at() const {
            return *std::next(this->children.begin(), N);
        }

        Vec<2> offset_of_child(size_t child_i) const;
        bool update_root(
            const Window& window, f64 unit
        );
        void render_root(
            Manager& manager, Scene& scene, const Window& window, f64 unit
        );

        Element& with_padding(LayoutFunc amount);
        Element& with_padding(f64 units) {
            if(units == 0) { return *this; }
            return this->with_padding(unit * units);
        }


        private:
        void update_position(LayoutContext ctx);
        void update_size(LayoutContext ctx);
        void update_hovering(const Window& window);
        bool update_clicked(const Window& window);

        void render(Manager& manager, Scene& scene, f64 unit) const;
        void render_background(Manager& manager, Scene& scene, f64 unit) const;
        void render_text(Manager& manager, Scene& scene, f64 unit) const;

    };


    struct Manager {

        static const inline std::vector<Mesh::Attrib> mesh_attribs = {
            (Mesh::Attrib) { Mesh::F32, 2 }
        };

        static const inline Shader::LoadArgs shader_args = {
            "res/shaders/ui_vert.glsl", "res/shaders/ui_frag.glsl"
        };

        static void load_shaders(Scene& scene) {
            scene.load(Manager::shader_args);
        }


        private:
        Texture target = Texture(100, 100);
        Mesh quad = Mesh(Manager::mesh_attribs);
        Shader* shader;
        bool clicked = false;
        bool hovered = false;
        f64 unit_size_px = 0;

        public:
        Element root = Element()
            .with_pos(null, null)
            .with_size(width::window, height::window)
            .as_phantom() // still allows children to be clicked or hovered over
            .as_movable();
        f64 unit_fract_size; // fraction of window height

        Manager(f64 unit_fract_size);
        Manager& with_element(Element&& element) {
            this->root.children.push_back(std::move(element));
            return *this;
        }

        const engine::Texture& output() const { return this->target; }
        bool was_clicked() const { return this->clicked; }
        bool is_hovered_over() const { return this->hovered; }
        f64 px_per_unit() const { return this->unit_size_px; }

        void update(const Window& window);

        void render(Scene& scene, const Window& window);

        struct Instance {
            Vec<2> src_pos, src_size;
            Vec<2> dest_pos, dest_size;
        };

        void blit_texture(
            const Texture& src, std::span<const Instance> instances
        );

    };

}