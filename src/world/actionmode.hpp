
#pragma once

#include <engine/window.hpp>
#include <engine/ui.hpp>
#include "../renderer.hpp"
#include "../toasts.hpp"
#include "../ui_const.hpp"
#include "../audio_const.hpp"
#include "world.hpp"

namespace houseofatmos::world {

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

        static inline const engine::Shader::LoadArgs terrain_overlay_shader = {
            "res/shaders/terrain_overlay_vert.glsl", 
            "res/shaders/terrain_overlay_frag.glsl"
        };

        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Texture::Loader(wireframe_info_texture));
            scene.load(engine::Texture::Loader(wireframe_add_texture));
            scene.load(engine::Texture::Loader(wireframe_sub_texture));
            scene.load(engine::Texture::Loader(wireframe_valid_texture));
            scene.load(engine::Texture::Loader(wireframe_error_texture));
            scene.load(engine::Shader::Loader(terrain_overlay_shader));
        }


        public:
        std::shared_ptr<World>& world;
        ui::Manager& ui;
        Toasts& toasts;
        engine::Localization& local;
        bool permitted = true;

        ActionMode(
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        ): world(world), ui(ui), toasts(toasts), local(local) {}

        virtual ~ActionMode() = default;

        virtual void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) = 0;
        virtual void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) = 0;

    };


    struct DefaultMode: ActionMode {

        ui::Element* button = nullptr;

        DefaultMode(
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        ): ActionMode(world, ui, toasts, local) {
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
            Renderer& renderer
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
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        );

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
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

        struct ChunkOverlay {
            u64 x, z;
            engine::Mesh terrain;
        };

        std::vector<ChunkOverlay> chunk_overlays;
        i64 last_viewed_chunk_x = INT64_MIN, last_viewed_chunk_z = INT64_MIN;

        ConstructionMode(
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        );

        void collect_chunk_overlays(i64 viewed_chunk_x, i64 viewed_chunk_z);

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
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
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
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
            Renderer& renderer
        ) override;

    };


    struct DemolitionMode: ActionMode {

        struct Selection {
            enum Type { None, Building, Bridge };
            Type type;
            struct BuildingSelection {
                u64 tile_x, tile_z;
                u64 chunk_x, chunk_z;
                const world::Building* selected;
            };
            union {
                BuildingSelection building;
                const world::Bridge* bridge; 
            } value;
        };

        Selection selection;


        DemolitionMode(
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        ): ActionMode(world, ui, toasts, local) {
            this->selection.type = Selection::None;
        }

        void attempt_demolition(engine::Scene& scene);

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) override;

    };


    struct PathingMode: ActionMode {

        u64 selected_tile_x, selected_tile_z;

        PathingMode(
            std::shared_ptr<World>& world, ui::Manager& ui, Toasts& toasts,
            engine::Localization& local
        ): ActionMode(world, ui, toasts, local) {
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) override {
            (void) window;
            (void) scene;
            (void) renderer;
        }

    };

}