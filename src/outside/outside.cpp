
#include "outside.hpp"
#include <fstream>

namespace houseofatmos::outside {

    static void load_resources(engine::Scene& scene) {
        Renderer::load_shaders(scene);
        Terrain::load_resources(scene);
        Building::load_models(scene);
        Foliage::load_models(scene);
        Player::load_model(scene);
        ActionMode::load_resources(scene);
        Carriage::load_resources(scene);
    }

    Outside::Outside() {
        load_resources(*this);
        this->terrain.generate_elevation();
        this->terrain.generate_foliage();
        this->player.position = { 250, 0, 250 };
        this->action_mode = std::make_unique<DefaultMode>(
            this->terrain, this->complexes, this->carriages
        );
        this->balance.coins = 105000;
        this->carriages = CarriageManager(this->terrain);
    }


    static void save_game(engine::Arena buffer) {
        std::ofstream fout;
        fout.open(Outside::save_location, std::ios::binary | std::ios::out);
        fout.write((const char*) buffer.data().data(), buffer.data().size());
        fout.close();
        engine::info(
            "Saved game to '" + std::string(Outside::save_location) + "'. "
            "Deleting the file will reset your progress."
        );
    }

    static void update_player(
        engine::Window& window, Terrain& terrain, Player& player
    ) {
        player.update(window);
        bool in_coll = !terrain.valid_player_position(Player::collider.at(player.position));
        if(in_coll || terrain.valid_player_position(Player::collider.at(player.next_x()))) {
            player.proceed_x();
        }
        if(in_coll || terrain.valid_player_position(Player::collider.at(player.next_z()))) {
            player.proceed_z();
        }
        player.position.y() = std::max(
            terrain.elevation_at(player.position),
            -1.7
        );
        player.in_water = player.position.y() <= -1.5;
    }

    static void update_camera(
        engine::Window& window, Player& player, Camera& camera,
        Zoom::Level& zoom
    ) {
        if(window.was_pressed(engine::Key::Space)) {
            switch(zoom) {
                case Zoom::Near: zoom = Zoom::Far; break;
                case Zoom::Far: zoom = Zoom::Near; break;
                default: 
                    engine::error("Unhandled 'Zoom::Level' in 'update_camera'");
            }
        }
        camera.look_at = player.position;
        f64 offset = Zoom::offset_of(zoom);
        camera.position = player.position + Vec<3>(0, offset, offset);
    }

    void Outside::update(engine::Window& window) {
        this->carriages.update_all(window, this->complexes, this->terrain);
        this->complexes.update(window, this->balance);
        ActionMode::choose_current(
            window, 
            this->terrain, this->complexes, this->player, this->carriages, 
            this->action_mode
        );
        this->action_mode->update(window, *this, this->renderer, this->balance);
        update_player(window, this->terrain, this->player);
        update_camera(window, this->player, this->renderer.camera, this->zoom);
        if(window.is_down(engine::Key::LeftControl) && window.was_pressed(engine::Key::S)) {
            save_game(this->serialize());
        }
    }

    void Outside::render(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->player.position);
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->player.render(*this, this->renderer);
        this->carriages.render_all(this->renderer, *this, window);
        this->action_mode->render(window, *this, this->renderer);
        window.show_texture(this->renderer.output());
    }


    Outside::Outside(const engine::Arena& buffer) {
        load_resources(*this);
        const auto& outside = buffer.value_at<Outside::Serialized>(0);
        this->terrain = Terrain(
            outside.terrain, 
            Outside::draw_distance, Outside::units_per_tile, Outside::tiles_per_chunk,
            buffer
        );
        this->complexes = ComplexBank(outside.complexes, buffer);
        this->player = Player(outside.player, buffer);
        this->action_mode = std::make_unique<DefaultMode>(
            this->terrain, this->complexes, this->carriages
        );
        this->balance = outside.balance;
        this->carriages = CarriageManager(
            outside.carriages, buffer, this->terrain
        );
    }

    engine::Arena Outside::serialize() const {
        auto buffer = engine::Arena();
        // we need to allocate the base struct first so that it's always at offset 0
        size_t outside_offset = buffer.alloc_array<Outside::Serialized>(nullptr, 1);
        Terrain::Serialized terrain = this->terrain.serialize(buffer);
        ComplexBank::Serialized complexes = this->complexes.serialize(buffer);
        Player::Serialized player = this->player.serialize(buffer);
        CarriageManager::Serialized carriages = this->carriages.serialize(buffer);
        auto& outside = buffer.value_at<Outside::Serialized>(outside_offset);
        outside.terrain = terrain;
        outside.complexes = complexes;
        outside.player = player;
        outside.balance = this->balance;
        outside.carriages = carriages;
        return buffer;
    }

}