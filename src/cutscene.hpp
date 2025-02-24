
#pragma once

#include <engine/window.hpp>
#include <functional>

namespace houseofatmos {

    struct Cutscene {

        using Handler = std::function<void (engine::Window&)>;

        struct Section {
            f64 remaining;
            Handler on_update;
            Handler on_end;

            Section(f64 duration, Handler&& on_update, Handler&& on_end): 
                remaining(duration), 
                on_update(std::move(on_update)), on_end(std::move(on_end)) {}
        };

        private:
        std::vector<Section> sections;

        public:
        Cutscene() {}
        Cutscene(std::initializer_list<Section> sections): sections(sections) {}

        void append(Cutscene&& other) {
            this->sections.insert(
                this->sections.end(), 
                other.sections.begin(), other.sections.end()
            );
        }
        
        void advance() {
            if(this->is_empty()) { return; }
            this->sections.at(0).remaining = 0.0;
        }

        const Section& current_section() const { return this->sections.at(0); }
        bool is_empty() const { return this->sections.size() == 0; }

        void update(engine::Window& window) {
            if(this->is_empty()) { return; }
            this->current_section().on_update(window);
            if(this->is_empty()) { return; }
            Section& section = this->sections.at(0);
            section.remaining -= window.delta_time();
            if(section.remaining > 0.0) { return; }
            section.on_end(window);
            if(this->is_empty()) { return; }
            this->sections.erase(this->sections.begin());
        }

    };

}