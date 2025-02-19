
#include "audio_const.hpp"

namespace houseofatmos::voice {

    #define MAKE_VOICED(c, f) \
        { (c), { ("res/sounds/dialogue_voiced/" f), 1.0, 0.05 } }

    const Dialogue::Voice voiced = {
        { // sounds specified for each letter
            { U' ', { "res/sounds/silence.ogg", 1.0, 0.0 } },
            { U'\n', { "res/sounds/silence.ogg", 1.0, 0.0 } },

            MAKE_VOICED(U'A', "a.ogg"), MAKE_VOICED(U'B', "b.ogg"),
            MAKE_VOICED(U'C', "c.ogg"), MAKE_VOICED(U'D', "d.ogg"),
            MAKE_VOICED(U'E', "e.ogg"), MAKE_VOICED(U'F', "f.ogg"),
            MAKE_VOICED(U'G', "g.ogg"), MAKE_VOICED(U'H', "h.ogg"),
            MAKE_VOICED(U'I', "i.ogg"), MAKE_VOICED(U'J', "j.ogg"),
            MAKE_VOICED(U'K', "k.ogg"), MAKE_VOICED(U'L', "l.ogg"),
            MAKE_VOICED(U'M', "m.ogg"), MAKE_VOICED(U'N', "n.ogg"),
            MAKE_VOICED(U'O', "o.ogg"), MAKE_VOICED(U'P', "p.ogg"),
            MAKE_VOICED(U'Q', "q.ogg"), MAKE_VOICED(U'R', "r.ogg"),
            MAKE_VOICED(U'S', "s.ogg"), MAKE_VOICED(U'T', "t.ogg"),
            MAKE_VOICED(U'U', "u.ogg"), MAKE_VOICED(U'V', "v.ogg"),
            MAKE_VOICED(U'W', "w.ogg"), MAKE_VOICED(U'X', "x.ogg"),
            MAKE_VOICED(U'Y', "y.ogg"), MAKE_VOICED(U'Z', "z.ogg"),

            MAKE_VOICED(U'a', "a.ogg"), MAKE_VOICED(U'b', "b.ogg"),
            MAKE_VOICED(U'c', "c.ogg"), MAKE_VOICED(U'd', "d.ogg"),
            MAKE_VOICED(U'e', "e.ogg"), MAKE_VOICED(U'f', "f.ogg"),
            MAKE_VOICED(U'g', "g.ogg"), MAKE_VOICED(U'h', "h.ogg"),
            MAKE_VOICED(U'i', "i.ogg"), MAKE_VOICED(U'j', "j.ogg"),
            MAKE_VOICED(U'k', "k.ogg"), MAKE_VOICED(U'l', "l.ogg"),
            MAKE_VOICED(U'm', "m.ogg"), MAKE_VOICED(U'n', "n.ogg"),
            MAKE_VOICED(U'o', "o.ogg"), MAKE_VOICED(U'p', "p.ogg"),
            MAKE_VOICED(U'q', "q.ogg"), MAKE_VOICED(U'r', "r.ogg"),
            MAKE_VOICED(U's', "s.ogg"), MAKE_VOICED(U't', "t.ogg"),
            MAKE_VOICED(U'u', "u.ogg"), MAKE_VOICED(U'v', "v.ogg"),
            MAKE_VOICED(U'w', "w.ogg"), MAKE_VOICED(U'x', "x.ogg"),
            MAKE_VOICED(U'y', "y.ogg"), MAKE_VOICED(U'z', "z.ogg"),
        },
        { "res/sounds/dialogue_pop.ogg", 1.0, 0.05 }, // default fallback sound
        0.1 // base duration of each charater
    };

    const Dialogue::Voice popped = {
        { // sounds specified for each letter
            { U' ', { "res/sounds/silence.ogg", 1.0, 0.0 } },
            { U'\n', { "res/sounds/silence.ogg", 1.0, 0.0 } },
        },
        { "res/sounds/dialogue_pop.ogg", 1.0, 0.05 }, // default fallback sound
        0.1 // base duration of each charater
    };

}