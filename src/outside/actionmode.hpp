
#pragma once

#include <engine/window.hpp>
#include <engine/ui.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "../toasts.hpp"
#include "../ui_const.hpp"
#include "../audio_const.hpp"
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
        engine::Scene& scene;
        Terrain& terrain;
        ComplexBank& complexes;
        CarriageManager& carriages;
        Player& player;
        Balance& balance;
        ui::Manager& ui;
        Toasts& toasts;
        const engine::Localization* local;

        ActionMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): scene(scene), terrain(terrain), complexes(complexes), 
            carriages(carriages), player(player), balance(balance), ui(ui), 
            toasts(toasts) {
            this->local = &toasts.localization();
        }

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

        ui::Element* button = nullptr;

        DefaultMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(scene, terrain, complexes, carriages, player, balance, ui, toasts) {
            this->ui.root.children.push_back(
                ui::Element().as_phantom().as_movable()
            );
            this->button = &this->ui.root.children.back();
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


    struct TerraformMode: ActionMode {

        struct Selection {
            u64 start_x, start_z;
            u64 end_x, end_z;
        };

        enum Mode {
            Flatten = 0, Raise = 1, Lower = 2,
            TotalCount = 3
        };

        static inline const std::vector<const ui::Background*> mode_icons = {
            &ui_icon::terrain_flatten, 
            &ui_icon::terrain_raise, 
            &ui_icon::terrain_lower
        };

        bool has_selection;
        Selection selection;
        std::unique_ptr<Mode> mode;

        std::unique_ptr<std::array<ui::Element*, (size_t) Mode::TotalCount>> mode_buttons;
        ui::Element* vertex_markers = nullptr;


        TerraformMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        );

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


    struct ConstructionMode: ActionMode {

        u64 selected_x, selected_z;
        std::unique_ptr<Building::Type> selected_type;
        std::unique_ptr<std::vector<Conversion>> selected_conversion;
        bool placement_valid;

        ConstructionMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        );

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct BridgingMode: ActionMode {

        struct Selection {
            u64 start_x, start_z;
            u64 end_x, end_z;
        };

        bool has_selection;
        Selection selection = { 0, 0, 0, 0 };
        std::unique_ptr<Bridge::Type> selected_type;

        Bridge planned;
        bool placement_valid;

        BridgingMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        );

        Bridge get_planned() const;
        i64 get_reduced_planned_height(
            i64 start, i64 (*reduce)(i64 acc, i64 height)
        ) const;
        bool planned_ends_match() const;
        bool planned_is_occupied() const;

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

        struct Selection {
            enum Type { None, Building, Bridge };
            Type type;
            struct BuildingSelection {
                u64 tile_x, tile_z;
                u64 chunk_x, chunk_z;
                const outside::Building* selected;
            };
            union {
                BuildingSelection building;
                const outside::Bridge* bridge; 
            } value;
        };

        Selection selection;


        DemolitionMode(
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(scene, terrain, complexes, carriages, player, balance, ui, toasts) {
            this->selection.type = Selection::None;
        }

        void attempt_demolition();

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
            engine::Scene& scene, Terrain& terrain, ComplexBank& complexes, 
            CarriageManager& carriages, Player& player, Balance& balance,
            ui::Manager& ui, Toasts& toasts
        ): ActionMode(scene, terrain, complexes, carriages, player, balance, ui, toasts) {
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