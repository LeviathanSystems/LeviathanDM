#ifndef TILING_LAYOUT_HPP
#define TILING_LAYOUT_HPP

#include "Types.hpp"
#include <wlr/util/box.h>
#include <vector>

namespace Leviathan {

class TilingLayout {
public:
    TilingLayout();
    
    // Apply different layouts
    void ApplyMasterStack(std::vector<Wayland::View*>& views,
                         int master_count, float master_ratio,
                         int screen_width, int screen_height,
                         int gap_size);
    
    void ApplyMonocle(std::vector<Wayland::View*>& views,
                     int screen_width, int screen_height);
    
    void ApplyGrid(std::vector<Wayland::View*>& views,
                  int screen_width, int screen_height,
                  int gap_size);
    
private:
    void MoveResizeView(Wayland::View* view, 
                       int x, int y, 
                       int width, int height);
};

} // namespace Leviathan

#endif // TILING_LAYOUT_HPP
