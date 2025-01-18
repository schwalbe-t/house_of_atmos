
#include <engine/ui.hpp>

namespace houseofatmos::engine::ui {


    static void default_click_handler() {}

    Element::Element() {
        this->size_px = Vec<2>();
        this->position_px = Vec<2>();
        this->hovering = false;
        this->with_size(10, 10, size::units);
        this->with_pos(0, 0, position::parent_units);
        this->with_children(std::vector<Element>(), Direction::Vertical);
        this->with_background(nullptr, nullptr);
        this->with_texture(nullptr);
        this->with_click_handler(default_click_handler);
        this->with_text("");
        this->hidden = false;
    }


    Vec<2> Element::offset_of_child(size_t child_i) const {
        Vec<2> offset;
        for(size_t i = 0; i < child_i; i += 1) {
            const Element& child = this->children[i];
            if(child.hidden) { continue; }
            offset += child.size_px;
        }
        switch(this->list_direction) {
            case Direction::Horizontal: return Vec<2>(offset.x(), 0);
            case Direction::Vertical: return Vec<2>(0, offset.y());
        }
        engine::error("Unhandled 'Direction' in 'Element::child_offset'!");
    }

    void Element::update_root(const Window& window, f64 unit) {
        this->update_size(nullptr, window, unit);
        this->update_position(std::nullopt, window, unit);
        this->update_input(window);
    }

    void Element::render_root(Manager& manager, Scene& scene, f64 unit) const {
        if(this->hidden) { return; }
        this->render_background(manager, scene, unit);
        for(const Element& child: this->children) {
            child.render_root(manager, scene, unit);
        }
    }


    void Element::update_size(
        const Element* parent, const Window& window, f64 unit
    ) {
        if(this->hidden) { return; }
        this->size_px = this->size_func(*this, parent, window, unit);
        for(Element& child: this->children) {
            child.update_size(this, window, unit);
        }
    }

    void Element::update_position(
        std::optional<ChildRef> parent, const Window& window, f64 unit
    ) {
        if(this->hidden) { return; }
        this->position_px = this->pos_func(*this, parent, window, unit);
        for(size_t child_i = 0; child_i < this->children.size(); child_i += 1) {
            Element& child = this->children[child_i];
            auto child_ref = (ChildRef) { *this, child_i };
            child.update_position(child_ref, window, unit);
        }
    }

    void Element::update_input(const Window& window) {
        if(this->hidden) { return; }
        Vec<2> cursor = window.cursor_pos_px();
        this->hovering = this->position_px.x() <= cursor.x()
            && cursor.x() < this->position_px.x() + this->size_px.x()
            && this->position_px.y() <= cursor.y()
            && cursor.y() < this->position_px.y() + this->size_px.y();
        if(this->hovering && window.was_pressed(Button::Left)) {
            this->on_click();
        }
        for(Element& child: this->children) {
            child.update_input(window);
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
        const Texture& tex = scene.get<Texture>(bkg->texture);
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
        manager.blit_texture(tex, instances);
    }     



    Vec<2> position::window_tl_units(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        (void) window;
        return self.position * unit;
    }

    Vec<2> position::window_tr_units(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        return Vec<2>(
            window.width() - self.position.x() * unit,
            self.position.y() * unit
        );
    }

    Vec<2> position::window_bl_units(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        return Vec<2>(
            self.position.x() * unit,
            window.height() - self.position.y() * unit
        );
    }

    Vec<2> position::window_br_units(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        return window.size() - (self.position * unit);
    }

    Vec<2> position::window_fract(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        (void) unit;
        return (window.size() - self.final_size()) * self.position;
    }

    Vec<2> position::window_ndc(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        (void) unit;
        return Vec<2>(
            window.width() * (self.position.x() + 1.0) / 2.0,
            window.height() * (1.0 - ((self.position.y() + 1.0) / 2.0))
        );
    }

    Vec<2> position::parent_units(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) window;
        if(!parent.has_value()) {
            engine::error("Cannot use 'position::parent_units' with root element!");
        }
        return parent->element.final_pos()
            + parent->element.offset_of_child(parent->child_i)
            + (self.position * unit);
    }

    Vec<2> position::parent_fract(
        const Element& self, std::optional<ChildRef> parent, 
        const Window& window, f64 unit
    ) {
        (void) window;
        (void) unit;
        if(!parent.has_value()) {
            engine::error("Cannot use 'position::parent_fract' with root element!");
        }
        return parent->element.final_pos()
            + parent->element.offset_of_child(parent->child_i)
            + (parent->element.final_size() - self.final_size()) * self.position;
    }



    Vec<2> size::units(
        const Element& self, const Element* parent, 
        const Window& window, f64 unit
    ) {
        (void) parent;
        (void) window;
        return self.size * unit;
    }

    Vec<2> size::parent_fract(
        const Element& self, const Element* parent, 
        const Window& window, f64 unit
    ) {
        (void) unit;
        (void) window;
        if(parent == nullptr) {
            engine::error("Cannot use 'size::parent_fract' with root element!");
        }
        return parent->final_size() * self.size;
    }

    Vec<2> size::window_fract(
        const Element& self, const Element* parent, 
        const Window& window, f64 unit
    ) {
        (void) unit;
        (void) parent;
        return window.size() * self.size;
    }



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

    void Manager::update(const Window& window) {
        this->root.update_root(window, window.height() * this->unit_fract_size);
    }

    void Manager::render(const Window& window, Scene& scene) {
        if(window.width() > 0 && window.height() > 0) {
            this->target.resize_fast(window.width(), window.height());
        }
        this->target.clear_color(Vec<4>(0.0, 0.0, 0.0, 0.0));
        this->shader = &scene.get<Shader>(Manager::shader_args);
        this->root.render_root(
            *this, scene, window.height() * this->unit_fract_size
        );
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
            src_scales.push_back(instance.src_size / src_tex_size);
            src_offsets.push_back(instance.src_pos / src_tex_size);
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
            this->quad.render(*this->shader, this->target, count, false, false);
            completed += count;
        }
    }

}