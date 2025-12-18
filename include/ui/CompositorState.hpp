#ifndef UI_COMPOSITOR_STATE_HPP
#define UI_COMPOSITOR_STATE_HPP

#include <string>
#include <vector>
#include <memory>

namespace Leviathan {

// Forward declarations - plugins don't need full definitions
namespace Core {
    class Screen;
    class Tag;
    class Client;
}

namespace UI {

/**
 * CompositorState provides read-only access to compositor state for widgets
 * This allows widgets to query current screens, tags, clients without
 * coupling them to the Server implementation.
 */
class CompositorState {
public:
    virtual ~CompositorState() = default;
    
    // Screen queries
    virtual std::vector<Core::Screen*> GetScreens() const = 0;
    virtual Core::Screen* GetFocusedScreen() const = 0;
    
    // Tag queries
    virtual std::vector<Core::Tag*> GetTags() const = 0;
    virtual Core::Tag* GetActiveTag() const = 0;  // Tag on focused screen
    
    // Client queries
    virtual std::vector<Core::Client*> GetAllClients() const = 0;
    virtual std::vector<Core::Client*> GetClientsOnTag(Core::Tag* tag) const = 0;
    virtual std::vector<Core::Client*> GetClientsOnScreen(Core::Screen* screen) const = 0;
    virtual Core::Client* GetFocusedClient() const = 0;
};

/**
 * Global accessor for compositor state
 * Widgets can use this to query current compositor state
 */
CompositorState* GetCompositorState();
void SetCompositorState(CompositorState* state);

} // namespace UI
} // namespace Leviathan

#endif // UI_COMPOSITOR_STATE_HPP
