
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

        using AbstractAgent = void*;

        struct AgentDisplay {
            struct Button {
                std::string_view local_text;
                bool (*show_if)(AbstractAgent, World&);
                void (*handler)(AbstractAgent, World&, Toasts&);
            };

            const ui::Background* marker;
            std::string_view local_title;
            std::string_view local_remove;
            std::vector<AgentStop>& (*schedule_of)(AbstractAgent);
            u64 (*target_stop_of)(AbstractAgent);
            Vec<3> (*position_of)(AbstractAgent, World&);
            AgentState (*state_of)(AbstractAgent);
            const std::unordered_map<Item::Type, u64>& 
                (*items_of)(AbstractAgent);
            const ui::Background* (*icon_of)(AbstractAgent);
            std::string_view (*local_name_of)(AbstractAgent);
            u64 (*item_capacity_of)(AbstractAgent);
            void (*reset_path_of)(AbstractAgent, World&);
            void (*remove)(AbstractAgent, World&);
            std::vector<Button> buttons;
        };

        template<typename N>
        static inline std::vector<AgentStop>& schedule_of_agent(
            AbstractAgent agent
        ) { return ((Agent<N>*) agent)->schedule; }

        template<typename N>
        static inline u64 target_stop_of_agent(AbstractAgent agent) { 
            return ((Agent<N>*) agent)->stop_i; 
        }

        template<typename N>
        static inline Vec<3> position_of_agent(
            AbstractAgent agent, N& network
        ) { 
            return ((Agent<N>*) agent)->current_position(network); 
        }

        template<typename N>
        static inline AgentState state_of_agent(AbstractAgent agent) { 
            return ((Agent<N>*) agent)->current_state(); 
        }

        template<typename N>
        static inline const std::unordered_map<Item::Type, u64>& items_of_agent(
            AbstractAgent agent
        ) { return ((Agent<N>*) agent)->items; }

        template<typename N>
        static inline const ui::Background* icon_of_agent(AbstractAgent agent) { 
            return ((Agent<N>*) agent)->icon(); 
        }

        template<typename N>
        static inline std::string_view local_name_of_agent(
            AbstractAgent agent
        ) { return ((Agent<N>*) agent)->local_name(); }

        template<typename N>
        static inline u64 item_capacity_of_agent(AbstractAgent agent) { 
            return ((Agent<N>*) agent)->item_storage_capacity(); 
        }

        template<typename N>
        static inline void reset_path_of_agent(
            AbstractAgent agent, N& network
        ) {
            Agent<N>* t_agent = (Agent<N>*) agent;
            if(t_agent->schedule.size() < 2) { return; }
            t_agent->travel_to(network, t_agent->next_stop().target);
        }

        template<typename A>
        static inline void remove_agent(
            AbstractAgent removed, std::list<A>& agents
        ) {
            auto current = agents.begin();
            while(current != agents.end()) {
                A& agent = *current;
                if((AbstractAgent) (&agent) == removed) {
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
            &TerrainMap::schedule_of_agent<CarriageNetwork>,
            &TerrainMap::target_stop_of_agent<CarriageNetwork>,
            +[](AbstractAgent a, World& w) {
                return TerrainMap::position_of_agent<CarriageNetwork>(
                    a, w.carriages.network
                );
            },
            &TerrainMap::state_of_agent<CarriageNetwork>,
            &TerrainMap::items_of_agent<CarriageNetwork>,
            &TerrainMap::icon_of_agent<CarriageNetwork>,
            &TerrainMap::local_name_of_agent<CarriageNetwork>,
            &TerrainMap::item_capacity_of_agent<CarriageNetwork>,
            +[](AbstractAgent a, World& w) {
                TerrainMap::reset_path_of_agent<CarriageNetwork>(
                    a, w.carriages.network
                );
            },
            +[](AbstractAgent a, World& w) {
                TerrainMap::remove_agent<Carriage>(a, w.carriages.agents);
            },
            {}
        };

        static inline AgentDisplay train_display = {
            &ui_icon::map_marker_train,
            "ui_train", "ui_remove_train",
            &TerrainMap::schedule_of_agent<TrackNetwork>,
            &TerrainMap::target_stop_of_agent<TrackNetwork>,
            +[](AbstractAgent a, World& w) {
                return TerrainMap
                    ::position_of_agent<TrackNetwork>(a, w.trains.network);
            },
            &TerrainMap::state_of_agent<TrackNetwork>,
            &TerrainMap::items_of_agent<TrackNetwork>,
            &TerrainMap::icon_of_agent<TrackNetwork>,
            &TerrainMap::local_name_of_agent<TrackNetwork>,
            &TerrainMap::item_capacity_of_agent<TrackNetwork>,
            +[](AbstractAgent a, World& w) {
                TerrainMap
                    ::reset_path_of_agent<TrackNetwork>(a, w.trains.network);
            },
            +[](AbstractAgent a, World& w) {
                TerrainMap::remove_agent<Train>(a, w.trains.agents);
            },
            {
                AgentDisplay::Button(
                    "ui_train_add_car", 
                    +[](AbstractAgent agent, World& world) {
                        (void) world;
                        auto t_agent = (Agent<TrackNetwork>*) agent;
                        auto train = dynamic_cast<Train*>(t_agent);
                        const auto& loco_info = Train::locomotive_types()
                            .at((size_t) train->loco_type);
                        return train->cars.size() - loco_info.loco_cars.size()
                            < loco_info.max_car_count;
                    },
                    +[](AbstractAgent agent, World& w, Toasts& t) {
                        auto t_agent = (Agent<TrackNetwork>*) agent;
                        auto train = dynamic_cast<Train*>(t_agent);
                        bool append = w.balance
                            .pay_coins(Train::train_car_cost, t);
                        if(!append) { return; }
                        train->cars.push_back(train->cars.back());
                    }
                ),
                AgentDisplay::Button(
                    "ui_train_remove_car", 
                    +[](AbstractAgent agent, World& world) {
                        (void) world;
                        auto t_agent = (Agent<TrackNetwork>*) agent;
                        auto train = dynamic_cast<Train*>(t_agent);
                        const auto& loco_info = Train::locomotive_types()
                            .at((size_t) train->loco_type);
                        return train->cars.size() > loco_info.loco_cars.size();
                    },
                    +[](AbstractAgent agent, World& world, Toasts& toasts) {
                        (void) world;
                        (void) toasts;
                        auto t_agent = (Agent<TrackNetwork>*) agent;
                        auto train = dynamic_cast<Train*>(t_agent);
                        train->cars.pop_back();
                    }
                )
            }
        };

        static inline AgentDisplay boat_display = {
            &ui_icon::map_marker_boat,
            "ui_boat", "ui_remove_boat",
            &TerrainMap::schedule_of_agent<BoatNetwork>,
            &TerrainMap::target_stop_of_agent<BoatNetwork>,
            +[](AbstractAgent a, World& w) {
                return TerrainMap
                    ::position_of_agent<BoatNetwork>(a, w.boats.network);
            },
            &TerrainMap::state_of_agent<BoatNetwork>,
            &TerrainMap::items_of_agent<BoatNetwork>,
            &TerrainMap::icon_of_agent<BoatNetwork>,
            &TerrainMap::local_name_of_agent<BoatNetwork>,
            &TerrainMap::item_capacity_of_agent<BoatNetwork>,
            +[](AbstractAgent a, World& w) {
                TerrainMap
                    ::reset_path_of_agent<BoatNetwork>(a, w.boats.network);
            },
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
            AbstractAgent agent, const AgentDisplay& agent_display,
            std::span<const size_t> stop_indices
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

        static ui::Element display_item_selector(
            std::span<const Item::Type> items, 
            std::function<void (Item::Type)>&& handler,
            const engine::Localization& local
        );

        ui::Element display_agent_info(
            AbstractAgent agent, const AgentDisplay& agent_display
        );

        ui::Element display_agent_stop(
            AbstractAgent agent, const AgentDisplay& agent_display,
            size_t stop_i
        );

        ui::Element display_agent_details(
            AbstractAgent agent, const AgentDisplay& agent_display
        );

    };

}