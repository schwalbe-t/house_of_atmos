
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
    }


    Vec<2> Element::offset_of_child(size_t child_i) const {
        Vec<2> offset;
        for(size_t i = 0; i < child_i; i += 1) {
            const Element& child = this->children[i];
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

    void Element::render_root() {
        // TODO!
    }


    void Element::update_size(
        const Element* parent, const Window& window, f64 unit
    ) {
        this->size_px = this->size_func(*this, parent, window, unit);
        for(Element& child: this->children) {
            child.update_size(this, window, unit);
        }
    }

    void Element::update_position(
        std::optional<ChildRef> parent, const Window& window, f64 unit
    ) {
        this->position_px = this->pos_func(*this, parent, window, unit);
        for(size_t child_i = 0; child_i < this->children.size(); child_i += 1) {
            Element& child = this->children[child_i];
            auto child_ref = (ChildRef) { *this, child_i };
            child.update_position(child_ref, window, unit);
        }
    }

    void Element::update_input(const Window& window) {
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

}