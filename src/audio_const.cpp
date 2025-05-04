
#include "audio_const.hpp"

namespace houseofatmos::voice {

    #define MAKE_VOICED(c, f) { \
        (c), engine::Sound::LoadArgs(("res/sounds/dialogue_voiced/" f), 0.05) \
    }

    const Dialogue::Voice voiced = {
        { // sounds specified for each letter
            { U' ', engine::Sound::LoadArgs("res/sounds/silence.ogg", 0.0) },
            { U'\n', engine::Sound::LoadArgs("res/sounds/silence.ogg", 0.0) },

            MAKE_VOICED(U'A', "a.ogg"), MAKE_VOICED(U'a', "a.ogg"),
            MAKE_VOICED(U'B', "b.ogg"), MAKE_VOICED(U'b', "b.ogg"),
            MAKE_VOICED(U'C', "c.ogg"), MAKE_VOICED(U'c', "c.ogg"),
            MAKE_VOICED(U'D', "d.ogg"), MAKE_VOICED(U'd', "d.ogg"),
            MAKE_VOICED(U'E', "e.ogg"), MAKE_VOICED(U'e', "e.ogg"),
            MAKE_VOICED(U'F', "f.ogg"), MAKE_VOICED(U'f', "f.ogg"),
            MAKE_VOICED(U'G', "g.ogg"), MAKE_VOICED(U'g', "g.ogg"),
            MAKE_VOICED(U'H', "h.ogg"), MAKE_VOICED(U'h', "h.ogg"),
            MAKE_VOICED(U'I', "i.ogg"), MAKE_VOICED(U'i', "i.ogg"),
            MAKE_VOICED(U'J', "j.ogg"), MAKE_VOICED(U'j', "j.ogg"),
            MAKE_VOICED(U'K', "k.ogg"), MAKE_VOICED(U'k', "k.ogg"), 
            MAKE_VOICED(U'L', "l.ogg"), MAKE_VOICED(U'l', "l.ogg"),
            MAKE_VOICED(U'M', "m.ogg"), MAKE_VOICED(U'm', "m.ogg"),
            MAKE_VOICED(U'N', "n.ogg"), MAKE_VOICED(U'n', "n.ogg"),
            MAKE_VOICED(U'O', "o.ogg"), MAKE_VOICED(U'o', "o.ogg"),
            MAKE_VOICED(U'P', "p.ogg"), MAKE_VOICED(U'p', "p.ogg"),
            MAKE_VOICED(U'Q', "q.ogg"), MAKE_VOICED(U'q', "q.ogg"),
            MAKE_VOICED(U'R', "r.ogg"), MAKE_VOICED(U'r', "r.ogg"),
            MAKE_VOICED(U'S', "s.ogg"), MAKE_VOICED(U's', "s.ogg"),
            MAKE_VOICED(U'T', "t.ogg"), MAKE_VOICED(U't', "t.ogg"),
            MAKE_VOICED(U'U', "u.ogg"), MAKE_VOICED(U'u', "u.ogg"),
            MAKE_VOICED(U'V', "v.ogg"), MAKE_VOICED(U'v', "v.ogg"),
            MAKE_VOICED(U'W', "w.ogg"), MAKE_VOICED(U'w', "w.ogg"),
            MAKE_VOICED(U'X', "x.ogg"), MAKE_VOICED(U'x', "x.ogg"),
            MAKE_VOICED(U'Y', "y.ogg"), MAKE_VOICED(U'y', "y.ogg"),
            MAKE_VOICED(U'Z', "z.ogg"), MAKE_VOICED(U'z', "z.ogg"),

            MAKE_VOICED(U'Ӓ', "a.ogg"), MAKE_VOICED(U'ä', "a.ogg"),
            MAKE_VOICED(U'Ö', "o.ogg"), MAKE_VOICED(U'ö', "o.ogg"),
            MAKE_VOICED(U'Ü', "u.ogg"), MAKE_VOICED(U'ü', "u.ogg"),
            MAKE_VOICED(U'ẞ', "s.ogg"), MAKE_VOICED(U'ß', "s.ogg"),

            MAKE_VOICED(U'А', "a.ogg"), MAKE_VOICED(U'а', "a.ogg"),
            MAKE_VOICED(U'Б', "b.ogg"), MAKE_VOICED(U'б', "b.ogg"),
            MAKE_VOICED(U'В', "w.ogg"), MAKE_VOICED(U'в', "w.ogg"),
            MAKE_VOICED(U'Г', "g.ogg"), MAKE_VOICED(U'г', "g.ogg"),
            MAKE_VOICED(U'Д', "d.ogg"), MAKE_VOICED(U'д', "d.ogg"),
            MAKE_VOICED(U'Е', "e.ogg"), MAKE_VOICED(U'е', "e.ogg"),
            MAKE_VOICED(U'Ж', "j.ogg"), MAKE_VOICED(U'ж', "j.ogg"),
            MAKE_VOICED(U'З', "z.ogg"), MAKE_VOICED(U'з', "z.ogg"),
            MAKE_VOICED(U'И', "i.ogg"), MAKE_VOICED(U'и', "i.ogg"),
            MAKE_VOICED(U'Й', "i.ogg"), MAKE_VOICED(U'й', "i.ogg"),
            MAKE_VOICED(U'К', "k.ogg"), MAKE_VOICED(U'к', "k.ogg"),
            MAKE_VOICED(U'Л', "l.ogg"), MAKE_VOICED(U'л', "l.ogg"),
            MAKE_VOICED(U'М', "m.ogg"), MAKE_VOICED(U'м', "m.ogg"),
            MAKE_VOICED(U'Н', "n.ogg"), MAKE_VOICED(U'н', "n.ogg"),
            MAKE_VOICED(U'О', "o.ogg"), MAKE_VOICED(U'о', "o.ogg"),
            MAKE_VOICED(U'П', "p.ogg"), MAKE_VOICED(U'п', "p.ogg"),
            MAKE_VOICED(U'Р', "r.ogg"), MAKE_VOICED(U'р', "r.ogg"),
            MAKE_VOICED(U'С', "s.ogg"), MAKE_VOICED(U'с', "s.ogg"),
            MAKE_VOICED(U'Т', "t.ogg"), MAKE_VOICED(U'т', "t.ogg"),
            MAKE_VOICED(U'У', "u.ogg"), MAKE_VOICED(U'у', "u.ogg"),
            MAKE_VOICED(U'Ф', "f.ogg"), MAKE_VOICED(U'ф', "f.ogg"),
            MAKE_VOICED(U'Х', "r.ogg"), MAKE_VOICED(U'х', "r.ogg"),
            MAKE_VOICED(U'Ц', "c.ogg"), MAKE_VOICED(U'ц', "c.ogg"),
            MAKE_VOICED(U'Ч', "j.ogg"), MAKE_VOICED(U'ч', "j.ogg"),
            MAKE_VOICED(U'Ш', "j.ogg"), MAKE_VOICED(U'ш', "j.ogg"),
            MAKE_VOICED(U'Щ', "j.ogg"), MAKE_VOICED(U'щ', "j.ogg"),
            MAKE_VOICED(U'Ъ', "o.ogg"), MAKE_VOICED(U'ъ', "o.ogg"),
            MAKE_VOICED(U'Ю', "u.ogg"), MAKE_VOICED(U'ю', "u.ogg"),
            MAKE_VOICED(U'Я', "a.ogg"), MAKE_VOICED(U'я', "a.ogg"),
            MAKE_VOICED(U'ь', "e.ogg"),
            MAKE_VOICED(U'ѝ', "i.ogg"),
        },
        // default fallback sound
        engine::Sound::LoadArgs("res/sounds/dialogue_pop.ogg", 0.05),
        0.1 // base duration of each charater
    };

    const Dialogue::Voice popped = {
        { // sounds specified for each letter
            { U' ', engine::Sound::LoadArgs("res/sounds/silence.ogg", 0.0) },
            { U'\n', engine::Sound::LoadArgs("res/sounds/silence.ogg", 0.0) },
        },
        // default fallback sound
        engine::Sound::LoadArgs("res/sounds/dialogue_pop.ogg", 0.05),
        0.1 // base duration of each character
    };

}


namespace houseofatmos::audio_const {

    const Soundtrack::LoadArgs soundtrack = Soundtrack::LoadArgs(
        {
            "res/soundtrack/track_1.ogg",
            "res/soundtrack/track_2.ogg",
            "res/soundtrack/track_3.ogg"
        },
        Soundtrack::Repetition::Forbidden
    );

}