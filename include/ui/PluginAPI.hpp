#ifndef UI_PLUGIN_API_HPP
#define UI_PLUGIN_API_HPP

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * LEVIATHAN PLUGIN API - SINGLE HEADER FOR PLUGIN DEVELOPMENT
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * This is the ONLY header plugins need to include.
 * 
 * PROVIDES:
 *   - WidgetPlugin base class for creating widgets
 *   - CompositorState interface for querying compositor
 *   - Helper functions for accessing compositor state (C-style API)
 *   - Export macros (EXPORT_PLUGIN_CREATE, etc.)
 * 
 * USAGE:
 *   #include "ui/PluginAPI.hpp"
 *   
 *   class MyWidget : public Leviathan::UI::WidgetPlugin {
 *       void Update() override {
 *           auto* state = Leviathan::UI::GetCompositorState();
 *           
 *           // Query tags
 *           auto tags = state->GetTags();
 *           for (auto* tag : tags) {
 *               // Use helper functions to avoid needing full Tag class
 *               std::string name = Leviathan::UI::Plugin::GetTagName(tag);
 *               int client_count = Leviathan::UI::Plugin::GetTagClientCount(tag);
 *           }
 *       }
 *   };
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "ui/WidgetPlugin.hpp"
#include "ui/CompositorState.hpp"
#include <string>
#include <vector>

namespace Leviathan {
namespace Core {
    // Forward declarations - plugins work with opaque pointers
    class Tag;
    class Client;
    class Screen;
}

namespace UI {
namespace Plugin {

/**
 * Helper functions for querying compositor objects
 * These provide a C-style API so plugins don't need the full class definitions
 */

// Tag (workspace) queries
std::string GetTagName(Core::Tag* tag);
bool IsTagVisible(Core::Tag* tag);
int GetTagClientCount(Core::Tag* tag);
std::vector<Core::Client*> GetTagClients(Core::Tag* tag);

// Client (window) queries  
std::string GetClientTitle(Core::Client* client);
std::string GetClientAppId(Core::Client* client);
bool IsClientFloating(Core::Client* client);
bool IsClientFullscreen(Core::Client* client);

// Screen (monitor) queries
std::string GetScreenName(Core::Screen* screen);
int GetScreenWidth(Core::Screen* screen);
int GetScreenHeight(Core::Screen* screen);
Core::Tag* GetScreenCurrentTag(Core::Screen* screen);

} // namespace Plugin
} // namespace UI
} // namespace Leviathan

#endif // UI_PLUGIN_API_HPP
