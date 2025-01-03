
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
    }

    Outside::Outside() {
        load_resources(*this);
        this->terrain.generate_elevation();
        this->terrain.generate_foliage();
        this->player.position = { 250, 0, 250 };
        this->action_mode = std::make_unique<DefaultMode>();
        this->balance.coins = 105000;
        this->balance.population = 0;
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

    void Outside::update(engine::Window& window) {
        this->complexes.update(window, this->balance);
        ActionMode::choose_current(window, this->terrain, this->complexes, this->player, this->action_mode);
        this->action_mode->update(window, *this, this->renderer, this->balance);
        this->player.update(window);
        bool in_coll = !this->terrain.valid_player_position(Player::collider.at(this->player.position));
        if(in_coll || this->terrain.valid_player_position(Player::collider.at(this->player.next_x()))) {
            this->player.proceed_x();
        }
        if(in_coll || this->terrain.valid_player_position(Player::collider.at(this->player.next_z()))) {
            this->player.proceed_z();
        }
        this->player.position.y() = std::max(
            this->terrain.elevation_at(this->player.position),
            -1.7
        );
        this->player.in_water = this->player.position.y() <= -1.5;
        this->renderer.camera.look_at = this->player.position;
        this->renderer.camera.position = this->player.position
            + Vec<3>(0, 12, 12);
        if(window.is_down(engine::Key::LeftControl) && window.was_pressed(engine::Key::S)) {
            save_game(this->serialize());
        }
    }

    void Outside::render(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->player.position);
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->player.render(*this, this->renderer);
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
        this->action_mode = std::make_unique<DefaultMode>();
        this->balance = outside.balance;
    }

    engine::Arena Outside::serialize() const {
        auto buffer = engine::Arena();
        // we need to allocate the base struct first so that it's always at offset 0
        size_t outside_offset = buffer.alloc_array<Outside::Serialized>(nullptr, 1);
        Terrain::Serialized terrain = this->terrain.serialize(buffer);
        ComplexBank::Serialized complexes = this->complexes.serialize(buffer);
        Player::Serialized player = this->player.serialize(buffer);
        auto& outside = buffer.value_at<Outside::Serialized>(outside_offset);
        outside.terrain = terrain;
        outside.complexes = complexes;
        outside.player = player;
        outside.balance = this->balance;
        return buffer;
    }

}