#ifndef CORE_TAG_HPP
#define CORE_TAG_HPP

#include "core/Client.hpp"
#include "Types.hpp"
#include <string>
#include <vector>

namespace Leviathan {
namespace Core {

class Client;

/**
 * Tag represents a workspace/collection of clients (windows)
 * - Can be displayed on any screen
 * - Contains clients (windows)
 * - Has its own layout settings
 * - Tags can be shown on multiple screens (unlike traditional workspaces)
 */
class Tag {
public:
    Tag(const std::string& name);
    ~Tag();
    
    // Tag properties
    const std::string& GetName() const { return name_; }
    bool IsVisible() const { return visible_; }
    void SetVisible(bool visible);
    
    // Client management
    void AddClient(Client* client);
    void RemoveClient(Client* client);
    const std::vector<Client*>& GetClients() const { return clients_; }
    Client* GetFocusedClient() const { return focused_client_; }
    void FocusClient(Client* client);
    
    // Layout settings
    LayoutType GetLayout() const { return layout_; }
    void SetLayout(LayoutType layout);
    int GetMasterCount() const { return master_count_; }
    void SetMasterCount(int count);
    float GetMasterRatio() const { return master_ratio_; }
    void SetMasterRatio(float ratio);
    
private:
    std::string name_;
    bool visible_;
    std::vector<Client*> clients_;
    Client* focused_client_;
    
    // Layout settings
    LayoutType layout_;
    int master_count_;
    float master_ratio_;
};

} // namespace Core
} // namespace Leviathan

#endif // CORE_TAG_HPP
