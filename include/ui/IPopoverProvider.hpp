#pragma once

#include <memory>

namespace Leviathan {
namespace UI {

// Forward declaration
class Popover;

/**
 * @brief Interface for widgets that have popovers
 * 
 * Widgets can implement this interface to expose their popover
 * for rendering by the StatusBar or other containers.
 */
class IPopoverProvider {
public:
    virtual ~IPopoverProvider() = default;
    
    virtual std::shared_ptr<Popover> GetPopover() const = 0;
    virtual bool HasPopover() const = 0;
};

} // namespace UI
} // namespace Leviathan
