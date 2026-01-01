#ifndef TYPES_HPP
#define TYPES_HPP

// C++ standard library
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace Leviathan {

// Forward declarations
class StatusBar;
struct WindowDecorationConfig;

namespace Core {
    class Seat;
    class Screen;
    class Tag;
}

namespace Wayland {
    struct View;  // Defined in wayland/View.hpp
    struct Output;  // Defined in wayland/Output.hpp
    class Server;
    class LayerManager;
}

// Layout types
enum class LayoutType {
    MASTER_STACK,
    MONOCLE,
    FLOATING,
    GRID
};

// Key modifier masks (using xkbcommon values)
enum Modifier {
    MOD_NONE = 0,
    MOD_SHIFT = (1 << 0),
    MOD_CTRL = (1 << 2),
    MOD_ALT = (1 << 3),
    MOD_SUPER = (1 << 6)
};

} // namespace Leviathan

#endif // TYPES_HPP
