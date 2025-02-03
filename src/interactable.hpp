
#pragma once

#include <engine/ui.hpp>
#include "renderer.hpp"
#include <functional>
#include <vector>
#include <memory>

namespace houseofatmos {

    using namespace houseofatmos::engine::math;
    namespace ui = houseofatmos::engine::ui;


    struct Interactables {

        struct Instance {

            friend class Interactables;

            public:
            std::function<void ()> handler;
            Vec<3> pos;

            protected:
            Instance(std::function<void ()>&& handler, Vec<3> pos) {
                this->pos = pos;
                this->handler = std::move(handler);
            }

        };

        private:
        std::vector<std::weak_ptr<Instance>> instances;
        ui::Element* container = nullptr;

        public:
        Interactables() {}

        ui::Element create_container();

        std::shared_ptr<Instance> create(
            std::function<void ()>&& handler, Vec<3> pos = { 0, 0, 0 }
        );

        void observe_from(
            const Vec<3>& pos, 
            const Renderer& renderer, const engine::Window& window
        );

        private:
        void clean_instance_refs();
        void update_ui_elements(
            const Vec<3>& pos, 
            const Renderer& renderer, const engine::Window& window
        );

    };

    using Interactable = Interactables::Instance;

}