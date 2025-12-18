#include "ui/CompositorState.hpp"

namespace Leviathan {
namespace UI {

// Global compositor state instance
static CompositorState* g_compositor_state = nullptr;

CompositorState* GetCompositorState() {
    return g_compositor_state;
}

void SetCompositorState(CompositorState* state) {
    g_compositor_state = state;
}

} // namespace UI
} // namespace Leviathan
