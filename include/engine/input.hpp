
#pragma once

#include "nums.hpp"

namespace houseofatmos::engine {
    
    // Sure, I could've just used the GLFW macros directly or used
    // them here, but don't want to always have that header imported,
    // so I'd rather do this
    // (this is an abstraction, after all)

    enum struct Key {
        // https://www.glfw.org/docs/latest/group__keys.html
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
        Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71,
        H = 72, I = 73, J = 74, K = 75, L = 76, M = 77, N = 78,
        O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85,
        V = 86, W = 87, X = 88, Y = 89, Z = 90,
        BracketOpen = 91,
        Backslash = 92,
        BracketClose = 93,
        GraveAccent = 96,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        ArrowRight = 262, ArrowLeft = 263,
        ArrowDown = 264, ArrowUp = 265,
        PageUp = 266, PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, 
        F6 = 295, F7 = 296, F8 = 297, F9 = 298, F10 = 299, 
        F11 = 300, F12 = 301, F13 = 302, F14 = 303, F15 = 304, 
        F16 = 305, F17 = 306, F18 = 307, F19 = 308, F20 = 309, 
        F21 = 310, F22 = 311, F23 = 312, F24 = 313, F25 = 314,
        KP0 = 320, KP1 = 321, KP2 = 322, KP3 = 323, KP4 = 324,
        KP5 = 325, KP6 = 326, KP7 = 327, KP8 = 328, KP9 = 329,
        KPDecimal = 330,
        KPDivide = 331, KPMultiply = 332,
        KPSubtract = 333, KPAdd = 334,
        KPEnter = 335,
        KPEqual = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,
        MaximumValue = Menu
    };

    
    enum struct Button {
        // https://www.glfw.org/docs/latest/group__buttons.html
        Mouse1 = 0,
        Mouse2 = 1,
        Mouse3 = 2,
        Mouse4 = 3,
        Mouse5 = 4,
        Mouse6 = 5,
        Mouse7 = 6,
        Mouse8 = 7,
        Left = Mouse1,
        Middle = Mouse2,
        Right = Mouse3,
        MaximumValue = Mouse8
    };

}