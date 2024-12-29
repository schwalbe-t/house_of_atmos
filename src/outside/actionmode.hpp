
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

        static inline const engine::Texture::LoadArgs wireframe_valid_texture = {
            "res/terrain/wireframe_valid.png"
        };

        static inline const engine::Texture::LoadArgs wireframe_error_texture = {
            "res/terrain/wireframe_error.png"
        };

        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Texture::Loader(ActionMode::wireframe_add_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_sub_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_valid_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_error_texture));
        }


        enum Type {
            Default = 0,
            Terraform = 1,
            Construction = 2
        };

        static inline const std::vector<engine::Key> keys = {
            engine::Key::Escape, // Default
            engine::Key::T, // Terraform
            engine::Key::C // Construction
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
                "Press T to enter terraforming mode "
                "and C to enter construction mode."
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
                "Left click to raise terrain, right click to lower terrain."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Terraform; }

        void update(
            const engine::Window& window, const Renderer& renderer,
            Balance& balance
        ) override;
        void render(engine::Scene& scene, const Renderer& renderer) override;

    };


    struct ConstructionMode: ActionMode {

        Terrain& terrain;
        u64 selected_x, selected_z;
        Building::Type selected_type;
        bool placement_valid;

        ConstructionMode(Terrain& terrain): terrain(terrain) {
            engine::info(
                "Entered construction mode. Press C or Escape to exit. "
                "Use the number buttons to select a building and left click to place it."
            );
            this->selected_type = Building::Farmland;
        }

        ActionMode::Type get_type() override { return ActionMode::Construction; }

        void update(
            const engine::Window& window, const Renderer& renderer,
            Balance& balance
        ) override;
        void render(engine::Scene& scene, const Renderer& renderer) override;

    };

}