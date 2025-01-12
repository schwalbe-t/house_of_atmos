
#pragma once

#include <engine/window.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "carriage.hpp"
#include "terrain.hpp"
#include "zoom.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct ActionMode {

        static inline const engine::Texture::LoadArgs wireframe_info_texture = {
            "res/terrain/wireframe_info.png"
        };

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
            scene.load(engine::Texture::Loader(ActionMode::wireframe_info_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_add_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_sub_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_valid_texture));
            scene.load(engine::Texture::Loader(ActionMode::wireframe_error_texture));
        }


        enum Type {
            Default = 0,
            Terraform = 1,
            Construction = 2,
            Demolition = 3,
            Pathing = 4
        };

        static inline const std::vector<engine::Key> keys = {
            engine::Key::Escape, // Default
            engine::Key::T, // Terraform
            engine::Key::C, // Construction
            engine::Key::R, // Demolition
            engine::Key::P  // Pathing
        };


        virtual ~ActionMode() = default;

        virtual Type get_type() = 0;
        virtual void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) = 0;
        virtual void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) = 0;

        static void choose_current(
            const engine::Window& window,
            Terrain& terrain, ComplexBank& complexes, const Player& player,
            CarriageManager& carriages,
            std::unique_ptr<ActionMode>& current
        );

    };


    struct DefaultMode: ActionMode {

        struct Selection {
            enum Type {
                None,
                Complex,
                Building
            };
            union Value {
                ComplexId complex;
                struct {
                    u64 x, z;
                } building;
            };
            Type type;
            Value value;
        };

        Terrain& terrain;
        ComplexBank& complexes;
        CarriageManager& carriages;
        Selection selected;

        DefaultMode(
            Terrain& terrain, ComplexBank& complexes, CarriageManager& carriages
        ): terrain(terrain), complexes(complexes), carriages(carriages) {
            engine::info(
                "Press space to zoom in/out. "
                "Left click to view information about a building or carriage. "
                "Press T to enter terraforming mode, "
                "C to enter construction mode, "
                "R to enter demolition mode and "
                "P to enter pathing mode."
            );
            this->selected.type = Selection::None;
        }

        ActionMode::Type get_type() override { return ActionMode::Default; }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) override;

        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct TerraformMode: ActionMode {

        Terrain& terrain;
        u64 selected_x, selected_z;
        bool modification_valid;

        TerraformMode(Terrain& terrain): terrain(terrain) {
            engine::info(
                "Entered terraform mode. Press T or Escape to exit. "
                "Left click to raise terrain, right click to lower terrain."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Terraform; }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct ConstructionMode: ActionMode {

        Terrain& terrain;
        ComplexBank& complexes;
        const Player& player;
        CarriageManager& carriages;
        u64 selected_x, selected_z;
        Building::Type selected_type;
        std::vector<Conversion> selected_conversion;
        bool placement_valid;

        ConstructionMode(
            Terrain& terrain, ComplexBank& complexes, const Player& player,
            CarriageManager& carriages
        ): terrain(terrain), complexes(complexes), 
            player(player), carriages(carriages) {
            this->selected_x = 0;
            this->selected_z = 0;
            this->selected_type = Building::House;
            this->placement_valid = false;
            engine::info(
                "Entered construction mode. Press C or Escape to exit. "
                "Press enter to select a building and left click to place it."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Construction; }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct DemolitionMode: ActionMode {

        Terrain& terrain; 
        ComplexBank& complexes;
        CarriageManager& carriages;
        u64 selected_tile_x, selected_tile_z;
        u64 selected_chunk_x, selected_chunk_z;
        const Building* selected;

        DemolitionMode(
            Terrain& terrain, ComplexBank& complexes, CarriageManager& carriages
        ): terrain(terrain), complexes(complexes), carriages(carriages) {
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
            this->selected_chunk_x = 0;
            this->selected_chunk_z = 0;
            this->selected = nullptr;
            engine::info(
                "Entered demolition mode. Press R or Escape to exit. "
                "Left click to destroy an existing building."
            );
        }

        ActionMode::Type get_type() override { return ActionMode::Demolition; }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct PathingMode: ActionMode {

        Terrain& terrain;
        ComplexBank& complexes;
        CarriageManager& carriages;
        u64 selected_tile_x, selected_tile_z;

        PathingMode(
            Terrain& terrain, ComplexBank& complexes, CarriageManager& carriages
        ): terrain(terrain), complexes(complexes), carriages(carriages) {
            engine::info(
                "Entered pathing mode. Press P or Escape to exit. "
                "Left to place a path, right click to remove an existing one."
            );
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
        }

        ActionMode::Type get_type() override { return ActionMode::Pathing; }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer, Balance& balance
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override {
            (void) window;
            (void) scene;
            (void) renderer;
        }

    };

}