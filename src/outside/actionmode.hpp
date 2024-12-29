
#pragma once

#include <engine/window.hpp>
#include "../renderer.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct ActionMode {

        enum Type {
            Default = 0,
            Terraform = 1
        };

        static inline const std::vector<engine::Key> keys = {
            engine::Key::Escape, // Default
            engine::Key::T // Terraform
        };


        virtual ~ActionMode() = default;

        virtual Type get_type() = 0;
        virtual void update(const engine::Window& window, const Renderer& renderer) = 0;
        virtual void render(engine::Scene& scene, const Renderer& renderer) = 0;

        static void choose_current(
            const engine::Window& window, Terrain& terrain, 
            std::unique_ptr<ActionMode>& current
        );

    };


    struct DefaultMode: ActionMode {

        DefaultMode() {
            engine::info(
                "Press T to enter terraform mode."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Default; }

        void update(const engine::Window& window, const Renderer& renderer) override {
            (void) window;
            (void) renderer;
        }

        void render(engine::Scene& scene, const Renderer& renderer) override {
            (void) scene;
            (void) renderer;
        }

    };


    struct TerraformMode: ActionMode {

        Terrain& terrain;

        TerraformMode(Terrain& terrain): terrain(terrain) {
            terrain.show_terrain_wireframe = true;
            engine::info(
                "Entered terraform mode. Press T or Escape to exit. "
                "Right click to raise terrain, left click to lower terrain."
            );
        }
        
        ~TerraformMode() override {
            this->terrain.show_terrain_wireframe = false;
        }

        ActionMode::Type get_type() override { return ActionMode::Terraform; }

        void update(const engine::Window& window, const Renderer& renderer) override;
        void render(engine::Scene& scene, const Renderer& renderer) override;

    };




}