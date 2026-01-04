#pragma once

#include <memory>
#include <ctime>
#include "config/ConfigParser.hpp"  // For NightLightConfig
#include "wayland/WaylandTypes.hpp"  // For wlroots types

namespace Leviathan {

/**
 * @brief Night Light feature - applies warm color temperature overlay
 * 
 * Creates a scene overlay that applies a warm tint during configured
 * night hours to reduce eye strain and improve sleep quality.
 * Integrates with LayerManager's NightLight layer to render above all content.
 */
class NightLight {
public:
    NightLight(struct wlr_scene_tree* parent_layer, const NightLightConfig& config);
    ~NightLight();

    // Update the night light state (call periodically)
    void Update();

    // Set output dimensions for proper layer sizing
    void SetOutputDimensions(int width, int height);

    // Configuration updates
    void SetEnabled(bool enabled);
    void SetTemperature(float temperature);
    void SetStrength(float strength);
    void SetSchedule(int start_hour, int start_minute, int end_hour, int end_minute);
    
    // Query current state
    bool IsEnabled() const { return config_.enabled; }
    bool IsActive() const { return is_active_; }
    float GetCurrentStrength() const { return current_strength_; }
    
    // Get scene layer for integration
    struct wlr_scene_tree* GetSceneTree() const { return night_light_tree_; }

private:
    // Check if current time is within night hours
    bool IsNightTime() const;
    
    // Calculate transition progress (0.0 = day, 1.0 = full night)
    float CalculateTransitionProgress() const;
    
    // Apply the color temperature effect
    void ApplyEffect(float strength);
    
    // Create the overlay rectangle
    void CreateOverlay();
    
    // Destroy the overlay
    void DestroyOverlay();
    
    // Convert color temperature to RGB
    void TemperatureToRGB(float temperature, float& r, float& g, float& b);

private:
    NightLightConfig config_;
    struct wlr_scene_tree* parent_layer_;
    struct wlr_scene_tree* night_light_tree_;
    struct wlr_scene_rect* overlay_rect_;
    
    int output_width_ = 1280;
    int output_height_ = 720;
    
    bool is_active_ = false;
    float current_strength_ = 0.0f;
    time_t last_update_ = 0;
};

} // namespace Leviathan
