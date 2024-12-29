
#pragma once

#include <engine/window.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct ActionMode {

        static inline const engine::Texture::LoadArgs wireframe_add_texture = {
            "res/terrain/wireframe_add.png"
        };

        static inline const engine::Texture::LoadArgs wireframe_sub_texture = {
            "res/terrain/wireframe_sub.png"
        };

        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Texture::Loader(ActionMode::wireframe_add_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_sub_texture));
        }


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
        virtual void update(
            const engine::Window& window, const Renderer& renderer, 
            Balance& balance
        ) = 0;
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

        void update(
            const engine::Window& window, const Renderer& renderer, 
            Balance& balance
        ) override {
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
        u64 selected_x, selected_z;

        TerraformMode(Terrain& terrain): terrain(terrain) {
            engine::info(
                "Entered terraform mode. Press T or Escape to exit. "
                "Right click to raise terrain, left click to lower terrain."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Terraform; }

        void update(
            const engine::Window& window, const Renderer& renderer,
            Balance& balance
        ) override;
        void render(engine::Scene& scene, const Renderer& renderer) override;

    };




}