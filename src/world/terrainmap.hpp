
#pragma once

#include <engine/ui.hpp>
#include <engine/rendering.hpp>
#include <engine/localization.hpp>
#include "world.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;


    struct TerrainMap {

        enum struct SelectionType {
            None, Complex, Agent
        };

        struct AgentDisplay {
            struct Button {
                std::string_view local_text;
                bool (*show_if)(AbstractAgent, World&);
                void (*handler)(AbstractAgent, World&, Toasts&);
            };

            const ui::Background* marker;
            std::string_view local_title;
            std::string_view local_remove;
            void (*remove_impl)(AbstractAgent, World&);
            std::vector<Button> buttons;
        };

        template<typename A>
        static inline void remove_agent(
            AbstractAgent removed, std::list<A>& agents
        ) {
            auto current = agents.begin();
            while(current != agents.end()) {
                A& agent = *current;
                if(agent.as_abstract().data != removed.data) {
                    current = std::next(current);
                    continue;
                }
                agents.erase(current);
                return;
            }
        }

        static inline AgentDisplay carriage_display = {
            &ui_icon::map_marker_carriage,
            "ui_carriage", "ui_remove_carriage",
            +[](AbstractAgent a, World& w) {
                TerrainMap::remove_agent<Carriage>(a, w.carriages.agents);
            },
            {}
        };

        static inline AgentDisplay train_display = {
            &ui_icon::map_marker_train,
            "ui_train", "ui_remove_train",
            +[](AbstractAgent a, World& w) {
                TerrainMap::remove_agent<Train>(a, w.trains.agents);
            },
            {
                AgentDisplay::Button(
                    "ui_train_add_car", 
                    +[](AbstractAgent agent, World& world) {
                        (void) world;
                        auto t_agent = (Agent<TrackNetwork>*) agent.data;
                        auto train = dynamic_cast<Train*>(t_agent);
                        const auto& loco_info = Train::locomotive_types()
                            .at((size_t) train->loco_type);
                        return train->car_count < loco_info.max_car_count;
                    },
                    +[](AbstractAgent agent, World& w, Toasts& t) {
                        auto t_agent = (Agent<TrackNetwork>*) agent.data;
                        auto train = dynamic_cast<Train*>(t_agent);
                        bool append = w.balance
                            .pay_coins(Train::train_car_cost, t);
                        if(append) { train->car_count += 1; }
                    }
                ),
                AgentDisplay::Button(
                    "ui_train_remove_car", 
                    +[](AbstractAgent agent, World& world) {
                        (void) world;
                        auto t_agent = (Agent<TrackNetwork>*) agent.data;
                        auto train = dynamic_cast<Train*>(t_agent);
                        return train->car_count > 0;
                    },
                    +[](AbstractAgent agent, World& world, Toasts& toasts) {
                        (void) world;
                        (void) toasts;
                        auto t_agent = (Agent<TrackNetwork>*) agent.data;
                        auto train = dynamic_cast<Train*>(t_agent);
                        train->car_count -= 1;
                    }
                )
            }
        };

        static inline AgentDisplay boat_display = {
            &ui_icon::map_marker_boat,
            "ui_boat", "ui_remove_boat",
            +[](AbstractAgent a, World& w) {
                TerrainMap::remove_agent<Boat>(a, w.boats.agents);
            },
            {}
        };

        private:
        u64 t_width, t_height;
        const engine::Localization::LoadArgs local_ref;
        const engine::Localization* local = nullptr;
        std::shared_ptr<World> world;
        ui::Manager& ui;
        Toasts& toasts;
        engine::Image rendered_img = engine::Image(0, 0);
        engine::Texture rendered_tex = engine::Texture(1, 1);
        engine::Texture output_tex = engine::Texture(1, 1);
        Vec<2> view_size_px;
        Vec<2> view_pos_px;

        std::optional<Vec<2>> view_anchor = std::nullopt;

        ui::Element* container = nullptr;
        ui::Element* selected_info_right = nullptr;
        ui::Element* selected_info_bottom = nullptr;

        SelectionType selected_type = SelectionType::None;
        union {
            ComplexId complex;
            struct {
                AbstractAgent a;
                const AgentDisplay* d;
            } agent;
        } selected;
        bool adding_stop = false;

        void update_view(const engine::Window& window);
        void update_click(const engine::Window& window);

        void render_view();
        void create_marker_info();
        void add_marker(Vec<2> pos, ui::Element&& element);
        void add_icon_marker(
            Vec<2> pos, const ui::Background* icon, 
            std::function<void ()>&& handler, bool is_phantom = false
        );
        void add_agent_stop_marker(
            AbstractAgent agent, std::span<const size_t> stop_indices
        );
        void add_agent_markers(
            AbstractAgent agent, const AgentDisplay& agent_display
        );
        void create_markers();

        public:
        Vec<2> view_pos = Vec<2>(0.5, 0.5);
        f64 view_scale = 1.0;

        TerrainMap(
            engine::Localization::LoadArgs local_ref,
            std::shared_ptr<World> world, ui::Manager& ui, Toasts& toasts
        ): local_ref(std::move(local_ref)), world(std::move(world)), 
                ui(ui), toasts(toasts) {
            this->t_width = this->world->terrain.width_in_tiles();
            this->t_height = this->world->terrain.height_in_tiles();
            this->rendered_img = engine::Image(this->t_width, this->t_height);
        }

        void create_container();
        ui::Element* element() const { return this->container; }

        void hide();
        bool toggle_with_key(engine::Key key, const engine::Window& window);
        void update(const engine::Window& window, engine::Scene& scene);
        void render_map();
        void render();



        static ui::Element display_item_stack(
            Item::Type type, const std::string& count, 
            const engine::Localization& local, f64* text_v_pad_out = nullptr
        );

        static ui::Element display_item_stack_list(
            std::span<const Item::Stack> stacks, 
            const engine::Localization& local,
            f64* text_v_pad_out = nullptr
        );

        static ui::Element display_conversion(
            const Conversion& conv, const engine::Localization& local
        );

        static ui::Element display_building_info(
            Building::Type type, std::span<const Conversion> conversions,
            const engine::Localization& local
        );

        static ui::Element display_complex_info(
            const Complex& complex, const engine::Localization& local,
            const Terrain& terrain
        );

        static ui::Element create_selection_container(std::string title);

        static ui::Element create_selection_item(
            const ui::Background* icon, std::string text, bool selected,
            std::function<void ()>&& handler
        );

        static ui::Element display_item_selector(
            std::span<const Item::Type> items, 
            std::function<void (Item::Type)>&& handler,
            const engine::Localization& local
        );

        static ui::Element display_agent_info(
            AbstractAgent agent, const engine::Localization& local
        );

        ui::Element display_agent_stop(AbstractAgent agent, size_t stop_i);

        ui::Element display_agent_details(
            AbstractAgent agent, const AgentDisplay& agent_display
        );

    };

}