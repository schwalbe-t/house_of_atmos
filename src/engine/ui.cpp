
#include <engine/ui.hpp>

namespace houseofatmos::engine::ui {


    static void default_click_handler(const ui::Element& e, Vec<2> c) { 
        (void) e; (void) c; 
    }

    Element::Element() {
        this->size_px = Vec<2>();
        this->position_px = Vec<2>();
        this->hovering = false;
        this->with_size(ui::width::children, ui::height::children);
        this->with_pos(ui::horiz::list, ui::vert::list);
        this->with_list_dir(Direction::Vertical);
        this->with_background(nullptr, nullptr);
        this->with_texture(nullptr);
        this->with_click_handler(default_click_handler);
        this->with_text("", nullptr, true);
        this->with_handle(nullptr);
        this->as_hidden(false);
        this->as_phantom(false);
    }

    static void inherit_element_values(Element& from, Element& to, bool to_exists) {
        if(&from == &to) { return; }
        if(to_exists && to.handle != nullptr) {
            // set the pointer to the old element to null, since the element
            // is overwritten
            // this will result in a segfault if dereferenced,
            // but at least it's easier to debug than if we just left it :)
            *to.handle = nullptr;    
            to.handle = nullptr;
        }
        to.width_func = from.width_func;
        to.height_func = from.height_func;
        to.horiz_func = from.horiz_func;
        to.vert_func = from.vert_func;
        to.children = std::move(from.children);
        to.list_direction = from.list_direction;
        to.background = from.background;
        to.background_hover = from.background_hover;
        to.texture = from.texture;
        to.on_click = std::move(from.on_click);
        to.handle_dragging = from.handle_dragging;
        to.text = std::move(from.text);
        to.font = from.font;
        to.wrap_text = from.wrap_text;
        to.hidden = from.hidden;
        to.phantom = from.phantom;
        to.handle = from.handle;
        from.handle = nullptr;
        if(to.handle != nullptr) {
            // point the pointer to the old element to this location, since
            // it has moved here
            *to.handle = &to;
        }
    }

    Element::Element(Element&& other) noexcept {
        inherit_element_values(other, *this, false);
    }

    Element& Element::operator=(Element&& other) noexcept {
        inherit_element_values(other, *this, true);
        return *this;
    }

    Element::~Element() {
        if(this->handle != nullptr) {
            // set the pointer to the old element to null, since the element
            // is overwritten
            // this will result in a segfault if dereferenced,
            // but at least it's easier to debug than if we just left it :)
            *this->handle = nullptr;    
            this->handle = nullptr;
        }
    }

    Element& Element::with_padding(LayoutFunc amount) {
        auto padding = Element()
            .with_size(
                width::children + (amount * 2),
                height::children + (amount * 2)
            )
            .with_pos(this->horiz_func, this->vert_func)
            .as_movable();
        padding.children.push_back(this
            ->with_pos(horiz::parent + amount, vert::parent + amount)
            .as_movable()
        );
        *this = std::move(padding);
        return *this;
    }


    Vec<2> Element::offset_of_child(size_t child_i) const {
        Vec<2> offset;
        u64 i_child_i = 0;
        for(const Element& child: this->children) {
            if(i_child_i >= child_i) { break; }
            if(!child.hidden) { 
                offset += child.size_px;
            }
            i_child_i += 1;
        }
        switch(this->list_direction) {
            case Direction::Horizontal: return Vec<2>(offset.x(), 0);
            case Direction::Vertical: return Vec<2>(0, offset.y());
        }
        engine::error("Unhandled 'Direction' in 'Element::child_offset'!");
    }

    bool Element::update_root(const Window& window, f64 unit) {
        auto ctx = LayoutContext(*this, window, unit);
        this->update_size(ctx);
        this->update_position(ctx);
        this->update_hovering(window);
        return this->update_clicked(window);
    }

    void Element::render_root(
        Manager& manager, Scene& scene, const Window& window, f64 unit
    ) {
        auto ctx = LayoutContext(*this, window, unit);
        this->update_size(ctx);
        this->update_position(ctx);
        this->update_hovering(window);
        this->render(manager, scene, unit);
    }


    void Element::update_size(LayoutContext ctx) {
        if(this->hidden) { return; }
        this->size_px.x() = this->width_func.impl(ctx);
        this->size_px.y() = this->height_func.impl(ctx);
        size_t child_i = 0;
        for(Element& child: this->children) {
            child.update_size(ctx.for_child(child, child_i));
            child_i += 1;
        }
        this->size_px.x() = this->width_func.impl(ctx);
        this->size_px.y() = this->height_func.impl(ctx);
    }

    void Element::update_position(LayoutContext ctx) {
        if(this->hidden) { return; }
        this->position_px.x() = this->horiz_func.impl(ctx);
        this->position_px.y() = this->vert_func.impl(ctx);
        size_t child_i = 0;
        for(Element& child: this->children) {
            child.update_position(ctx.for_child(child, child_i));
            child_i += 1;
        }
    }

    void Element::update_hovering(const Window& window) {
        if(this->hidden) {
            this->hovering = false;
            return;
        }
        Vec<2> cursor = window.cursor_pos_px();
        this->hovering = !this->phantom
            && this->position_px.x() <= cursor.x()
            && cursor.x() < this->position_px.x() + this->size_px.x()
            && this->position_px.y() <= cursor.y()
            && cursor.y() < this->position_px.y() + this->size_px.y();
        for(Element& child: this->children) {
            child.update_hovering(window);
        }
    }

    bool Element::update_clicked(const Window& window) {
        if(this->hidden) { return false; }
        for(Element& child: this->children) {
            if(child.update_clicked(window)) { return true; }
        }
        bool was_dragged = this->hovering && this->handle_dragging 
            && window.is_down(Button::Left);
        bool was_clicked = this->hovering 
            && window.was_pressed(Button::Left);
        bool handled = was_dragged || was_clicked;
        if(handled) {
            this->on_click(*this, window.cursor_pos_px() - this->position_px); 
        }
        return handled;
    }


    void Element::render(Manager& manager, Scene& scene, f64 unit) const {
        if(this->hidden) { return; }
        this->render_background(manager, scene, unit);
        this->render_text(manager, scene, unit);
        if(this->texture != nullptr) {
            std::array<Manager::Instance, 1> instance = { (Manager::Instance) {
                Vec<2>(0, 0), 
                Vec<2>(this->texture->width(), this->texture->height()),
                this->final_pos(), this->final_size()
            } };
            manager.blit_texture(*this->texture, instance);
        }
        for(const Element& child: this->children) {
            child.render(manager, scene, unit);
        }
    }

    static void render_background_corners(
        std::vector<Manager::Instance>& instances, f64 unit,
        const Vec<2>& final_pos, const Vec<2>& final_size,
        const Background& bkg
    ) {
        Vec<2> corner_size_px = bkg.corner_size * unit;
        Vec<2> corner_src_tl = bkg.offset;
        Vec<2> corner_dest_tl = final_pos - corner_size_px;
        instances.push_back((Manager::Instance) {
            corner_src_tl, bkg.corner_size, corner_dest_tl, corner_size_px
        });
        Vec<2> corner_src_tr = bkg.offset
            + Vec<2>(bkg.corner_size.x() + bkg.edge_size.x(), 0);
        Vec<2> corner_dest_tr = final_pos 
            + Vec<2>(final_size.x(), -corner_size_px.y());
        instances.push_back((Manager::Instance) {
            corner_src_tr, bkg.corner_size, corner_dest_tr, corner_size_px
        });
        Vec<2> corner_src_bl = bkg.offset
            + Vec<2>(0, bkg.corner_size.y() + bkg.edge_size.y());
        Vec<2> corner_dest_bl = final_pos 
            + Vec<2>(-corner_size_px.x(), final_size.y());
        instances.push_back((Manager::Instance) {
            corner_src_bl, bkg.corner_size, corner_dest_bl, corner_size_px
        });
        Vec<2> corner_src_br = bkg.offset + bkg.corner_size + bkg.edge_size;
        Vec<2> corner_dest_br = final_pos + final_size;
        instances.push_back((Manager::Instance) {
            corner_src_br, bkg.corner_size, corner_dest_br, corner_size_px
        });
    }

    static void render_texture_row(
        std::vector<Manager::Instance>& instances, 
        const Vec<2>& src_pos, f64 unit,
        const Vec<2>& start, 
        const Vec<2>& step, const Vec<2> bound,
        const Vec<2>& max_size
    ) {
        Vec<2> offset;
        while(offset.x() < bound.x() && offset.y() < bound.y()) {
            Vec<2> pos = start + offset;
            Vec<2> size = Vec<2>(
                std::min(max_size.x(), bound.x() - offset.x()),
                std::min(max_size.y(), bound.y() - offset.y())
            );
            instances.push_back((Manager::Instance) {
                src_pos, size / unit, pos, size
            });
            offset.x() += step.x();
            offset.y() += step.y();
        }
    }

    static void render_edges(
        std::vector<Manager::Instance>& instances, f64 unit,
        const Vec<2>& final_pos, const Vec<2>& final_size,
        const Background& bkg
    ) {
        Vec<2> edge_size_px_h = Vec<2>(bkg.edge_size.x(), bkg.corner_size.y())
            * unit;
        Vec<2> edge_size_px_v = Vec<2>(bkg.corner_size.x(), bkg.edge_size.y())
            * unit;
        Vec<2> step_h = Vec<2>(bkg.edge_size.x(), 0) * unit;
        Vec<2> step_v = Vec<2>(0, bkg.edge_size.y()) * unit;
        Vec<2> bounds_h = Vec<2>(final_size.x(), INFINITY);
        Vec<2> bounds_v = Vec<2>(INFINITY, final_size.y());
        // top edge
        Vec<2> edge_src_top = bkg.offset + Vec<2>(bkg.corner_size.x(), 0);
        Vec<2> edge_dest_top = final_pos
            - Vec<2>(0, bkg.corner_size.y() * unit);
        render_texture_row(
            instances, edge_src_top, unit, 
            edge_dest_top, step_h, bounds_h, edge_size_px_h
        );
        // left edge
        Vec<2> edge_src_left = bkg.offset + Vec<2>(0, bkg.corner_size.y());
        Vec<2> edge_dest_left = final_pos
            - Vec<2>(bkg.corner_size.x() * unit, 0);
        render_texture_row(
            instances, edge_src_left, unit, 
            edge_dest_left, step_v, bounds_v, edge_size_px_v
        );
        // right edge
        Vec<2> edge_src_right = bkg.offset 
            + Vec<2>(bkg.corner_size.x() + bkg.edge_size.x(), bkg.corner_size.y());
        Vec<2> edge_dest_right = final_pos
            + Vec<2>(final_size.x(), 0);
        render_texture_row(
            instances, edge_src_right, unit, 
            edge_dest_right, step_v, bounds_v, edge_size_px_v
        );
        // bottom edge
        Vec<2> edge_src_bottom = bkg.offset 
            + Vec<2>(bkg.corner_size.x(), bkg.corner_size.y() + bkg.edge_size.y());
        Vec<2> edge_dest_bottom = final_pos
            + Vec<2>(0, final_size.y());
        render_texture_row(
            instances, edge_src_bottom, unit, 
            edge_dest_bottom, step_h, bounds_h, edge_size_px_h
        );
    }

    static void render_inner_background(
        std::vector<Manager::Instance>& instances, f64 unit,
        const Vec<2>& final_pos, const Vec<2>& final_size,
        const Background& bkg
    ) {
        Vec<2> segment = bkg.edge_size * unit;
        for(f64 ox = 0.0; ox < final_size.x(); ox += segment.x()) {
            f64 width = std::min(segment.x(), final_size.x() - ox);
            render_texture_row(
                instances, bkg.offset + bkg.corner_size, unit,
                final_pos + Vec<2>(ox, 0), 
                Vec<2>(0, segment.y()), Vec<2>(INFINITY, final_size.y()),
                Vec<2>(width, segment.y())
            );
        }
    }

    void Element::render_background(
        Manager& manager, Scene& scene, f64 unit
    ) const {
        const Background* bkg = this->hovering
            ? this->background_hover : this->background;
        if(bkg == nullptr) { bkg = this->background; }
        if(bkg == nullptr) { return; }
        std::vector<Manager::Instance> instances; 
        render_background_corners(
            instances, unit, this->final_pos(), this->final_size(), *bkg
        );
        render_edges(
            instances, unit, this->final_pos(), this->final_size(), *bkg
        );
        render_inner_background(
            instances, unit, this->final_pos(), this->final_size(), *bkg
        );
        manager.blit_texture(scene.get(bkg->texture), instances);
    }    

    static size_t utf8_char_length(std::string_view text) {
        if(text.size() == 0) { return 0; }
        size_t length = 1;
        while((text[length] & 0b11000000) == 0b10000000) {
            length += 1;
        }
        return length;
    }

    static size_t utf8_find_char_pos(
        std::string_view text, std::string_view searched, 
        size_t* offset_bytes_o = nullptr
    ) {
        size_t offset = 0;
        for(size_t i = 0; offset < text.size(); i += 1) {
            size_t c_len = utf8_char_length(text.substr(offset));
            std::string_view c = text.substr(offset, c_len);
            if(c == searched) {
                if(offset_bytes_o != nullptr) { *offset_bytes_o = offset; }
                return i;
            }
            offset += c_len;
        }
        return std::string::npos;
    }

    static f64 find_next_space_distance(
        std::string_view text, const Font& font,
        size_t o
    ) {
        size_t sc_len = 0;
        f64 ns_dist = 0;
        for(size_t d = 0; d + o < text.size(); d += sc_len) {
            sc_len = utf8_char_length(text.substr(d + o));
            std::string_view c = text.substr(d + o, sc_len);
            size_t idx = utf8_find_char_pos(font.chars, c);
            if(idx == std::string::npos) { idx = 0; sc_len = 1; }
            if(d > 0 && sc_len == 1 && (c[0] == ' ' || c[0] == '\n')) {
                break;
            }
            if(d > 0) {
                ns_dist += font.char_padding;
            }
            ns_dist += font.char_widths[idx];
        }
        return ns_dist;
    }

    static Vec<2> find_text_bounds(
        const ui::Element& e, f64 unit, bool wrap_text
    ) {
        if(e.font == nullptr) { return Vec<2>(0, 0); }
        Vec<2> max_offset;
        Vec<2> char_offset;
        size_t c_len;
        for(size_t o = 0; o < e.text.size(); o += c_len) {
            c_len = utf8_char_length(std::string_view(e.text).substr(o));
            std::string_view c = std::string_view(e.text).substr(o, c_len);
            if(c_len == 1 && c[0] == ' ') {
                f64 ns_pos_x = char_offset.x()
                    + find_next_space_distance(e.text, *e.font, o);
                if(ns_pos_x * unit > e.final_size().x()) {
                    c = "\n";
                }
            }
            if(c_len == 1 && c[0] == '\n' && wrap_text) {
                char_offset.x() = 0.0;
                char_offset.y() += e.font->height + e.font->char_padding;
                continue;
            }
            size_t idx = utf8_find_char_pos(e.font->chars, c);
            if(idx == std::string::npos) { idx = 0; }
            f64 width = e.font->char_widths[idx];
            char_offset.x() += width + e.font->char_padding;
            max_offset.x() = std::max(max_offset.x(), char_offset.x());
            max_offset.y() = std::max(max_offset.y(), char_offset.y());
        }
        return Vec<2>(
            std::max((max_offset.x() - 1.0), 0.0) * unit,
            (max_offset.y() + e.font->height) * unit 
        );
    }

    void Element::render_text(
        Manager& manager, Scene& scene, f64 unit
    ) const {
        if(this->font == nullptr) { return; }
        if(!this->font->char_offsets_x.has_value()) { return; }
        if(this->text.size() == 0) { return; }
        Texture& tex = scene.get(this->font->texture);
        std::vector<Manager::Instance> instances;
        instances.reserve(this->text.size()); 
        Vec<2> char_offset;
        size_t c_len;
        for(size_t o = 0; o < this->text.size(); o += c_len) {
            c_len = utf8_char_length(std::string_view(this->text).substr(o));
            std::string_view c = std::string_view(this->text).substr(o, c_len);
            if(c_len == 1 && c[0] == ' ') {
                f64 ns_pos_x = char_offset.x()
                    + find_next_space_distance(this->text, *this->font, o);
                if(ns_pos_x * unit > this->final_size().x()) {
                    c = "\n";
                }
            }
            if(c_len == 1 && c[0] == '\n') {
                char_offset.x() = 0.0;
                char_offset.y() += this->font->height 
                    + this->font->char_padding;
                continue;
            }
            size_t idx = utf8_find_char_pos(this->font->chars, c);
            if(idx == std::string::npos) { idx = 0; }
            f64 width = this->font->char_widths[idx];
            f64 src_offset_x = this->font->char_offsets_x->at(idx);
            Vec<2> src_size = Vec<2>(width, this->font->height);
            instances.push_back((Manager::Instance) {
                this->font->offset + Vec<2>(src_offset_x, 0), 
                src_size,
                this->final_pos() + char_offset * unit,
                src_size * unit
            });
            char_offset.x() += width + font->char_padding;
        }
        manager.blit_texture(tex, instances);
    }     



    LayoutFunc null = LayoutFunc([](auto ctx) { (void) ctx; return 0.0; });

    LayoutFunc unit = LayoutFunc([](auto ctx) { return ctx.unit; });



    LayoutFunc width::parent = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error(
                "Cannot use 'width::parent' with the root element or "
                "an element whose parents' size depends on that of its children!"
            );
        }
        return ctx.parent->final_size().x();
    });

    LayoutFunc width::window = LayoutFunc([](auto ctx) {
        return ctx.window->width();
    });

    LayoutFunc width::children = LayoutFunc([](auto ctx) {
        if(ctx.self->list_direction == Direction::Horizontal) {
            return ctx.self->offset_of_child(ctx.self->children.size()).x();
        }
        f64 width = 0.0;
        for(Element& child: ctx.self->children) {
            width = std::max(width, child.final_size().x());
        }
        return width;
    });

    LayoutFunc width::text = LayoutFunc([](auto ctx) {
        return find_text_bounds(*ctx.self, ctx.unit, false).x();
    });



    LayoutFunc height::parent = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error(
                "Cannot use 'height::parent' with the root element or "
                "an element whose parents' size depends on that of its children!"
            );
        }
        return ctx.parent->final_size().y();
    });

    LayoutFunc height::window = LayoutFunc([](auto ctx) {
        return ctx.window->height();
    });

    LayoutFunc height::children = LayoutFunc([](auto ctx) {
        if(ctx.self->list_direction == Direction::Vertical) {
            return ctx.self->offset_of_child(ctx.self->children.size()).y();
        }
        f64 height = 0.0;
        for(Element& child: ctx.self->children) {
            height = std::max(height, child.final_size().y());
        }
        return height;
    });

    LayoutFunc height::text = LayoutFunc([](auto ctx) {
        return find_text_bounds(*ctx.self, ctx.unit, true).y();
    });
    


    LayoutFunc horiz::in_window_fract(f64 fract) {
        return (width::window - horiz::width) * fract;
    }

    LayoutFunc horiz::in_parent_fract(f64 fract) {
        return horiz::parent + (width::parent - horiz::width) * fract;
    }

    LayoutFunc horiz::window_ndc(f64 ndc) {
        return width::window * ((ndc + 1.0) * 0.5);
    }

    LayoutFunc horiz::parent = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error("Cannot use 'horiz::parent' with root element!");
        }
        return ctx.parent->final_pos().x();
    });

    LayoutFunc horiz::list = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error("Cannot use 'horiz::list' with root element!");
        }
        return ctx.parent->final_pos().x()
            + ctx.parent->offset_of_child(ctx.child_i).x();
    });

    LayoutFunc horiz::width = LayoutFunc([](auto ctx) {
        return ctx.self->final_size().x();
    });



    LayoutFunc vert::in_window_fract(f64 fract) {
        return (height::window - vert::height) * fract;
    }

    LayoutFunc vert::in_parent_fract(f64 fract) {
        return vert::parent + (height::parent - vert::height) * fract;
    }

    LayoutFunc vert::window_ndc(f64 ndc) {
        return height::window * (1.0 - ((ndc + 1.0) / 2.0));
    }

    LayoutFunc vert::parent = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error("Cannot use 'vert::parent' with root element!");
        }
        return ctx.parent->final_pos().y();
    });

    LayoutFunc vert::list = LayoutFunc([](auto ctx) {
        if(ctx.parent == nullptr) {
            engine::error("Cannot use 'vert::list' with root element!");
        }
        return ctx.parent->final_pos().y()
            + ctx.parent->offset_of_child(ctx.child_i).y();
    });

    LayoutFunc vert::height = LayoutFunc([](auto ctx) {
        return ctx.self->final_size().y();
    });



    Manager::Manager(f64 unit_fract_size) {
        this->unit_fract_size = unit_fract_size;
        this->quad.start_vertex();
            this->quad.put_f32({ 0, 1 });
        u16 tl = this->quad.complete_vertex();
        this->quad.start_vertex();
            this->quad.put_f32({ 1, 1 });
        u16 tr = this->quad.complete_vertex();
        this->quad.start_vertex();
            this->quad.put_f32({ 0, 0 });
        u16 bl = this->quad.complete_vertex();
        this->quad.start_vertex();
            this->quad.put_f32({ 1, 0 });
        u16 br = this->quad.complete_vertex();
        this->quad.add_element(tl, bl, br);
        this->quad.add_element(br, tr, tl);
        this->quad.submit();
    }

    static bool is_hovered_over_rec(const Element& element) {
        if(element.hidden) { return false; }
        if(element.is_hovered_over()) { return true; }
        for(const Element& child: element.children) {
            if(is_hovered_over_rec(child)) { return true; }
        }
        return false;
    }

    void Manager::update(const Window& window) {
        this->unit_size_px = window.height() * this->unit_fract_size;
        this->clicked = this->root.update_root(window, this->unit_size_px);
        this->hovered = is_hovered_over_rec(this->root);
    }

    void Manager::render(Scene& scene, const Window& window) {
        if(window.width() > 0 && window.height() > 0) {
            this->target.resize_fast(window.width(), window.height());
        }
        this->target.as_target().clear_color(Vec<4>(0.0, 0.0, 0.0, 0.0));
        this->shader = &scene.get(Manager::shader_args);
        this->unit_size_px = window.height() * this->unit_fract_size;
        this->root.render_root(*this, scene, window, this->unit_size_px);
    }

    static const size_t max_batch_size = 128;

    void Manager::blit_texture(
        const Texture& src, std::span<const Instance> instances
    ) {
        Vec<2> src_tex_size = { src.width(), src.height() };
        Vec<2> tgt_tex_size = { this->target.width(), this->target.height() };
        std::vector<Vec<2>> src_scales;
        std::vector<Vec<2>> src_offsets;
        std::vector<Vec<2>> dest_scales;
        std::vector<Vec<2>> dest_offsets;
        src_scales.reserve(instances.size());
        src_offsets.reserve(instances.size());
        dest_scales.reserve(instances.size());
        dest_offsets.reserve(instances.size());
        for(const Instance& instance: instances) {
            src_scales.push_back(instance.src_size
                / src_tex_size * Vec<2>( 1.0, -1.0)
            );
            src_offsets.push_back(instance.src_pos 
                / src_tex_size * Vec<2>( 1.0, -1.0) + Vec<2>(0.0, 1.0)
            );
            Vec<2> dest_size = instance.dest_size;
            dest_size.x() = ceil(dest_size.x());
            dest_size.y() = ceil(dest_size.y());
            dest_scales.push_back(dest_size 
                / tgt_tex_size * Vec<2>( 2.0, -2.0)
            );
            Vec<2> dest_pos = instance.dest_pos;
            dest_pos.x() = floor(dest_pos.x());
            dest_pos.y() = floor(dest_pos.y());
            dest_offsets.push_back(dest_pos 
                / tgt_tex_size * Vec<2>( 2.0, -2.0) + Vec<2>(-1.0,  1.0)
            );
        }
        this->shader->set_uniform("u_texture", src);
        for(size_t completed = 0; completed < instances.size();) {
            size_t remaining = instances.size() - completed;
            size_t count = std::min(remaining, max_batch_size);
            auto src_s = std::span(src_scales).subspan(completed, count);
            auto src_o = std::span(src_offsets).subspan(completed, count);
            auto dest_s = std::span(dest_scales).subspan(completed, count);
            auto dest_o = std::span(dest_offsets).subspan(completed, count);
            this->shader->set_uniform("u_src_scales", src_s);
            this->shader->set_uniform("u_src_offsets", src_o);
            this->shader->set_uniform("u_dest_scales", dest_s);
            this->shader->set_uniform("u_dest_offsets", dest_o);
            this->quad.render(
                *this->shader, this->target.as_target(), count, 
                FaceCulling::Disabled, DepthTesting::Disabled
            );
            completed += count;
        }
    }

}