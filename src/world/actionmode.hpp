
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


    struct ActionContext {
        std::shared_ptr<World>* world;
        ui::Manager* ui;
        Toasts* toasts;
        engine::Localization* local;
    };


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

        static inline const engine::Shader::LoadArgs path_overlay_shader = {
            "res/shaders/path_overlay_vert.glsl",
            "res/shaders/path_overlay_frag.glsl"
        };

        static void load_resources(engine::Scene& scene) {
            scene.load(wireframe_info_texture);
            scene.load(wireframe_add_texture);
            scene.load(wireframe_sub_texture);
            scene.load(wireframe_valid_texture);
            scene.load(wireframe_error_texture);
            scene.load(terrain_overlay_shader);
            scene.load(path_overlay_shader);
        }


        public:
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 20.0
        );
        std::shared_ptr<World>& world;
        ui::Manager& ui;
        Toasts& toasts;
        engine::Localization& local;
        bool permitted = true;

        ActionMode(ActionContext ctx): 
            world(*ctx.world), ui(*ctx.ui), 
            toasts(*ctx.toasts), local(*ctx.local) {
            this->speaker.volume = this->world->settings.sfx_volume;
        }

        virtual ~ActionMode() = default;

        virtual void init_ui() {}
        virtual void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) {
            (void) window;
            (void) scene;
            (void) renderer;
        }
        virtual void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) {
            (void) window;
            (void) scene;
            (void) renderer;
        }

    };


    struct DefaultMode: ActionMode {

        ui::Element* button = nullptr;

        DefaultMode(ActionContext ctx): ActionMode(ctx) {}

        void init_ui() override {
            this->ui.root.children.push_back(
                ui::Element().as_phantom().as_movable()
            );
            this->button = &this->ui.root.children.back();
        }

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

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

        static inline const std::array<const ui::Background*, 3> mode_icons = {
            &ui_icon::terrain_flatten, 
            &ui_icon::terrain_raise, 
            &ui_icon::terrain_lower
        };

        bool has_selection;
        Selection selection;
        Mode mode;

        ui::Element* mode_selection = nullptr;
        ui::Element* vertex_markers = nullptr;


        TerraformMode(ActionContext ctx);

        void set_mode(Mode mode);
        void init_ui() override;

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;

    };


    struct ConstructionMode: ActionMode {

        struct BuildingVariant {
            std::vector<Conversion> conversions = {};
            std::optional<Resource::Type> required_resource = std::nullopt;
            std::optional<research::Research::Advancement> required_advancement
                = std::nullopt;
        };

        u64 selected_x, selected_z;
        std::unique_ptr<Building::Type> selected_type;
        std::unique_ptr<const BuildingVariant*> selected_variant;
        bool placement_valid;

        struct ChunkOverlay {
            u64 x, z;
            engine::Mesh terrain;
        };

        std::vector<ChunkOverlay> chunk_overlays;
        i64 last_viewed_chunk_x = INT64_MIN, last_viewed_chunk_z = INT64_MIN;

        ConstructionMode(ActionContext ctx);

        void init_ui() override;

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

        BridgingMode(ActionContext ctx);

        void init_ui() override;

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


        DemolitionMode(ActionContext ctx): ActionMode(ctx) {
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
        std::optional<engine::Mesh> overlay;

        PathingMode(ActionContext ctx): ActionMode(ctx) {
            this->selected_tile_x = 0;
            this->selected_tile_z = 0;
            this->overlay = std::nullopt;
        }

        void update_overlay();

        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) override;
        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) override;

    };



    struct ActionManager {

        enum struct Mode {
            Default, Terraform, Construction, Bridging, Demolition, Pathing
        };

        private:
        std::function<ActionContext ()> ctx;
        std::unique_ptr<ActionMode> mode = nullptr;
        Mode mode_type;
        bool was_changed = true;
        bool ui_initialized = false;

        public:
        ActionManager(std::function<ActionContext ()>&& ctx):
            ctx(std::move(ctx)) {}

        bool has_changed() const { return this->was_changed; }
        bool has_mode() const { return this->mode != nullptr; }
        ActionMode& current() {
            if(!this->has_mode()) {
                engine::error(
                    "Attempted access to mode of empty action mode manager"
                );
            }
            return *this->mode;
        }
        Mode current_type() const {
            if(!this->has_mode()) {
                engine::error(
                    "Attempted access to mode of empty action mode manager"
                );
            }
            return this->mode_type;
        }

        void acknowledge_change() { this->was_changed = false; }

        void set_mode(Mode mode) {
            ActionContext ctx = this->ctx();
            this->mode_type = mode;
            switch(mode) {
                case Mode::Default: 
                    this->mode = std::make_unique<DefaultMode>(ctx);
                    break;
                case Mode::Terraform:
                    this->mode = std::make_unique<TerraformMode>(ctx);
                    break;
                case Mode::Construction:
                    this->mode = std::make_unique<ConstructionMode>(ctx);
                    break;
                case Mode::Bridging:
                    this->mode = std::make_unique<BridgingMode>(ctx);
                    break;
                case Mode::Demolition:
                    this->mode = std::make_unique<DemolitionMode>(ctx);
                    break;
                case Mode::Pathing:
                    this->mode = std::make_unique<PathingMode>(ctx);
                    break;
            }
            this->was_changed = true;
            this->ui_initialized = false;
        }

        void remove_mode() {
            if(!this->has_mode()) { return; }
            this->mode = nullptr;
            this->was_changed = true;
        }

        void init_ui() {
            if(!this->has_mode()) { return; }
            if(this->ui_initialized) { return; }
            this->current().init_ui();
            this->ui_initialized = true;
        }
        
        void update(
            const engine::Window& window, engine::Scene& scene, 
            const Renderer& renderer
        ) {
            if(!this->has_mode() || !this->ui_initialized) { return; }
            this->mode->update(window, scene, renderer);
        }

        void render(
            const engine::Window& window, engine::Scene& scene, 
            Renderer& renderer
        ) {
            if(!this->has_mode() || !this->ui_initialized) { return; }
            this->mode->render(window, scene, renderer);
        }

    };

}