#ifndef CORE_SEAT_HPP
#define CORE_SEAT_HPP

#include "Screen.hpp"
#include "Tag.hpp"
#include "Client.hpp"
#include <vector>
#include <memory>

namespace Leviathan {
namespace Core {

/**
 * Seat represents the user's session
 * - Manages all screens (monitors)
 * - Manages all tags (workspaces)
 * - Manages all clients (windows)
 * - Handles global input
 * - Top-level coordinator
 */
class Seat {
public:
    Seat();
    ~Seat();
    
    // Screen management
    void AddScreen(Screen* screen);
    void RemoveScreen(Screen* screen);
    Screen* GetFocusedScreen();
    const std::vector<Screen*>& GetScreens() const { return screens_; }
    
    // Tag management
    void AddTag(Tag* tag);
    Tag* GetTag(int index);
    Tag* GetActiveTag();
    void SwitchToTag(int index);
    const std::vector<Tag*>& GetTags() const { return tags_; }
    
    // Client (window) management
    void AddClient(Client* client);
    void RemoveClient(Client* client);
    void MoveClientToTag(Client* client, int tag_index);
    Client* GetFocusedClient() const { return focused_client_; }
    void FocusClient(Client* client);
    const std::vector<Client*>& GetClients() const { return clients_; }
    
    // Focus navigation
    void FocusNextClient();
    void FocusPrevClient();
    void CloseClient(Client* client);
    
    // Layout control
    void TileCurrentTag();
    void SetLayout(LayoutType layout);
    
private:
    std::vector<Screen*> screens_;
    std::vector<Tag*> tags_;
    std::vector<Client*> clients_;
    Screen* focused_screen_;
    Client* focused_client_;
    int active_tag_index_;
};

} // namespace Core
} // namespace Leviathan

#endif // CORE_SEAT_HPP
