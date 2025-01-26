
#pragma once

#include <engine/window.hpp>
#include <engine/ui.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "../toasts.hpp"
#include "../ui_background.hpp"
#include "../ui_font.hpp"
#include "../ui_icon.hpp"
#include "carriage.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;
    namespace ui = houseofatmos::engine::ui;


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


        public:
        Terrain& terrain;
        ComplexBank& complexes;
        CarriageManager& carriages;
        Player& player;
        Balance& balance;
        ui::Manager& ui;
        Toasts& toasts;

        ActionMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): terrain(terrain), complexes(complexes), carriages(carriages),
            player(player), balance(balance), ui(ui), toasts(toasts) {}

        virtual ~ActionMode() = default;

        virtual void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) = 0;
        virtual void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) = 0;

    };


    struct DefaultMode: ActionMode {

        struct Selection {
            enum Type {
                None,
                Complex,
                Building,
                Carriage
            };
            union Value {
                ComplexId complex;
                struct {
                    u64 x, z;
                } building;
                u64 carriage;
            };
            Type type;
            Value value;
        };

        Selection selected;

        DefaultMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
            this->selected.type = Selection::None;
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct TerraformMode: ActionMode {

        u64 selected_x, selected_z;
        bool modification_valid;

        TerraformMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {}

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct ConstructionMode: ActionMode {

        u64 selected_x, selected_z;
        Building::Type selected_type;
        std::vector<Conversion> selected_conversion;
        bool placement_valid;

        ConstructionMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
            this->selected_x = 0;
            this->selected_z = 0;
            this->selected_type = Building::House;
            this->placement_valid = false;
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct DemolitionMode: ActionMode {

        u64 selected_tile_x, selected_tile_z;
        u64 selected_chunk_x, selected_chunk_z;
        const Building* selected;

        DemolitionMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
            this->selected_chunk_x = 0;
            this->selected_chunk_z = 0;
            this->selected = nullptr;
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct PathingMode: ActionMode {

        u64 selected_tile_x, selected_tile_z;

        PathingMode(
            Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
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