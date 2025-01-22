
#include "terrainmap.hpp"

namespace houseofatmos::outside {

    using Color = engine::Image::Color;


    static const Color high_grass_color = Color(192, 199, 65);
    static const Color low_grass_color = Color(98, 122, 51);
    static const Color sand_color = Color(245, 237, 186);
    static const Color high_water_color = Color(110, 169, 167);
    static const Color low_water_color = Color(44, 101, 118); 
    static const Color path_color = Color(154, 99, 72);
    static const Color building_color = Color(157, 48, 59);

    static const f64 step_size = 5;
    static const f64 grass_high = 25;
    static const f64 water_level = 0.0;
    static const f64 water_low = -25;

    static Color color_lerp(Color a, Color b, f64 n) {
        if(n < 0.0) { n = 0.0; }
        if(n > 1.0) { n = 1.0; }
        return Color(
            a.r + (u8) (n * (b.r - a.r)),
            a.g + (u8) (n * (b.g - a.g)),
            a.b + (u8) (n * (b.b - a.b)),
            a.a + (u8) (n * (b.a - a.a)) 
        );
    }

    void TerrainMap::render() {
        for(u64 x = 0; x < this->width; x += 1) {
            for(u64 z = 0; z < this->height; z += 1) {
                // get elevation data
                i64 elev_tl = this->terrain.elevation_at(x,     z    );
                i64 elev_tr = this->terrain.elevation_at(x + 1, z    );
                i64 elev_bl = this->terrain.elevation_at(x,     z + 1);
                i64 elev_br = this->terrain.elevation_at(x + 1, z + 1);
                f64 elev_avg = (elev_tl + elev_tr + elev_bl + elev_br) / 4.0;
                f64 step = trunc(elev_avg / step_size) * step_size;
                // compute pixel color
                Color grass_color = color_lerp(
                    low_grass_color, high_grass_color,
                    (step - water_level) / grass_high
                );
                Color water_color = color_lerp(
                    high_water_color, low_water_color,
                    (step - water_level) / water_low
                );
                Color color = grass_color;
                // if any vertex below water level, sand
                bool is_sand = elev_tl < 0 || elev_tr < 0
                    || elev_bl < 0 || elev_br < 0;
                if(is_sand) { color = sand_color; }
                // if all vertices below water level, water
                bool is_water = elev_tl < 0 && elev_tr < 0 
                    && elev_bl < 0 && elev_br < 0;
                if(is_water) { color = water_color; }
                // if is path or building, make building
                bool is_path = this->terrain.path_at((i64) x, (i64) z);
                if(is_path) { color = path_color; }
                bool is_building = this->terrain.building_at(x, z) != nullptr;
                if(is_building) { color = building_color; }
                // write computed color
                this->target.pixel_at(x, z) = color;
            }
        }
        this->result = std::move(engine::Texture(this->target));
    }


}