#include "wayland/NightLight.hpp"
#include "Logger.hpp"
#include <cmath>
#include <algorithm>

extern "C" {
#include <wlr/types/wlr_scene.h>
}

namespace Leviathan {

NightLight::NightLight(struct wlr_scene_tree* parent_layer, const NightLightConfig& config)
    : config_(config)
    , parent_layer_(parent_layer)
    , night_light_tree_(nullptr)
    , overlay_rect_(nullptr)
    , is_active_(false)
    , current_strength_(0.0f)
    , last_update_(0)
{
    LOG_INFO("=== NightLight Constructor ===");
    LOG_INFO_FMT("Config - enabled: {}, temp: {}, strength: {}", 
                 config_.enabled, config_.temperature, config_.strength);
    LOG_INFO_FMT("Schedule: {:02d}:{:02d} - {:02d}:{:02d}", 
                 config_.start_hour, config_.start_minute,
                 config_.end_hour, config_.end_minute);
    
    // Create a scene tree under the parent layer (NightLight layer from LayerManager)
    // This will be at the top of the scene graph
    night_light_tree_ = wlr_scene_tree_create(parent_layer_);
    
    if (night_light_tree_) {
        LOG_INFO("Created night light scene tree in NightLight layer");
        
        // Create the overlay rectangle
        CreateOverlay();
        
        // Initial update
        Update();
        
        LOG_INFO("NightLight initialization complete");
    } else {
        LOG_ERROR("Failed to create night light scene tree");
    }
}

NightLight::~NightLight() {
    DestroyOverlay();
    
    if (night_light_tree_) {
        wlr_scene_node_destroy(&night_light_tree_->node);
        night_light_tree_ = nullptr;
    }
}

void NightLight::SetOutputDimensions(int width, int height) {
    output_width_ = width;
    output_height_ = height;
    
    // Recreate overlay with new dimensions
    if (overlay_rect_) {
        DestroyOverlay();
        CreateOverlay();
        
        // Reapply current effect
        if (is_active_) {
            ApplyEffect(current_strength_);
        }
    }
    
    LOG_DEBUG_FMT("Night light output dimensions: {}x{}", width, height);
}

void NightLight::Update() {
    static int update_counter = 0;
    update_counter++;
    
    // Log every 60 updates (once per minute if called every second)
    if (update_counter % 60 == 1) {
        LOG_DEBUG_FMT("NightLight::Update() - enabled: {}, active: {}, strength: {:.2f}", 
                     config_.enabled, is_active_, current_strength_);
    }
    
    if (!config_.enabled) {
        if (is_active_) {
            // Disable night light
            is_active_ = false;
            current_strength_ = 0.0f;
            ApplyEffect(0.0f);
            LOG_INFO("Night light disabled");
        }
        return;
    }
    
    time_t now = time(nullptr);
    
    // Only update once per second to avoid excessive calculations
    if (now == last_update_) {
        return;
    }
    last_update_ = now;
    
    bool should_be_active = IsNightTime();
    float target_strength = 0.0f;
    
    if (should_be_active) {
        if (config_.smooth_transition) {
            target_strength = CalculateTransitionProgress();
        } else {
            target_strength = config_.strength;
        }
    }
    
    // Update state
    bool state_changed = (is_active_ != should_be_active);
    is_active_ = should_be_active;
    
    if (state_changed) {
        struct tm* timeinfo = localtime(&now);
        char time_str[16];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
        LOG_INFO_FMT("Night light {} at {} (should_be_active: {}, target_strength: {:.2f})", 
                    is_active_ ? "activated" : "deactivated",
                    time_str, should_be_active, target_strength);
    }
    
    // Apply effect if strength changed significantly (more than 1%)
    if (std::abs(current_strength_ - target_strength) > 0.01f) {
        current_strength_ = target_strength;
        ApplyEffect(current_strength_);
        
        if (current_strength_ > 0.0f) {
            LOG_INFO_FMT("Applying night light effect - strength: {:.2f}", current_strength_);
        }
    }
}

void NightLight::SetEnabled(bool enabled) {
    config_.enabled = enabled;
    Update();
}

void NightLight::SetTemperature(float temperature) {
    config_.temperature = std::max(1000.0f, std::min(6500.0f, temperature));
    if (is_active_) {
        ApplyEffect(current_strength_);
    }
}

void NightLight::SetStrength(float strength) {
    config_.strength = std::max(0.0f, std::min(1.0f, strength));
    Update();
}

void NightLight::SetSchedule(int start_hour, int start_minute, int end_hour, int end_minute) {
    config_.start_hour = start_hour;
    config_.start_minute = start_minute;
    config_.end_hour = end_hour;
    config_.end_minute = end_minute;
    Update();
}

bool NightLight::IsNightTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    int current_minutes = tm_now->tm_hour * 60 + tm_now->tm_min;
    int start_minutes = config_.start_hour * 60 + config_.start_minute;
    int end_minutes = config_.end_hour * 60 + config_.end_minute;
    
    // Handle overnight schedule (e.g., 20:00 to 06:00)
    if (start_minutes > end_minutes) {
        return current_minutes >= start_minutes || current_minutes < end_minutes;
    } else {
        // Same-day schedule (e.g., 08:00 to 17:00)
        return current_minutes >= start_minutes && current_minutes < end_minutes;
    }
}

float NightLight::CalculateTransitionProgress() const {
    if (config_.transition_duration <= 0) {
        return config_.strength;
    }
    
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    int current_minutes = tm_now->tm_hour * 60 + tm_now->tm_min;
    int start_minutes = config_.start_hour * 60 + config_.start_minute;
    int end_minutes = config_.end_hour * 60 + config_.end_minute;
    
    int transition_minutes = config_.transition_duration / 60;
    
    // Calculate minutes from start
    int minutes_from_start;
    if (start_minutes > end_minutes) {
        // Overnight schedule
        if (current_minutes >= start_minutes) {
            minutes_from_start = current_minutes - start_minutes;
        } else {
            minutes_from_start = (24 * 60 - start_minutes) + current_minutes;
        }
    } else {
        minutes_from_start = current_minutes - start_minutes;
    }
    
    // Fade in at start
    if (minutes_from_start < transition_minutes) {
        float progress = static_cast<float>(minutes_from_start) / transition_minutes;
        return progress * config_.strength;
    }
    
    // Calculate minutes until end
    int total_duration;
    if (start_minutes > end_minutes) {
        total_duration = (24 * 60 - start_minutes) + end_minutes;
    } else {
        total_duration = end_minutes - start_minutes;
    }
    
    int minutes_until_end = total_duration - minutes_from_start;
    
    // Fade out before end
    if (minutes_until_end < transition_minutes) {
        float progress = static_cast<float>(minutes_until_end) / transition_minutes;
        return progress * config_.strength;
    }
    
    // Full strength in the middle
    return config_.strength;
}

void NightLight::CreateOverlay() {
    if (!night_light_tree_) {
        return;
    }
    
    // Create a colored rectangle that covers the entire output
    overlay_rect_ = wlr_scene_rect_create(night_light_tree_, 
                                          output_width_, 
                                          output_height_, 
                                          (float[4]){0.0f, 0.0f, 0.0f, 0.0f});
    
    if (overlay_rect_) {
        // Initially invisible
        wlr_scene_node_set_enabled(&overlay_rect_->node, false);
        LOG_DEBUG_FMT("Created night light overlay rectangle: {}x{}", 
                     output_width_, output_height_);
    } else {
        LOG_ERROR("Failed to create night light overlay rectangle");
    }
}

void NightLight::DestroyOverlay() {
    if (overlay_rect_) {
        wlr_scene_node_destroy(&overlay_rect_->node);
        overlay_rect_ = nullptr;
    }
}

void NightLight::ApplyEffect(float strength) {
    if (!overlay_rect_) {
        LOG_ERROR("ApplyEffect called but overlay_rect_ is NULL!");
        return;
    }
    
    if (strength <= 0.0f) {
        // Disable overlay
        wlr_scene_node_set_enabled(&overlay_rect_->node, false);
        LOG_DEBUG("Disabled night light overlay (strength = 0)");
        return;
    }
    
    // Calculate RGB values from temperature
    float r, g, b;
    TemperatureToRGB(config_.temperature, r, g, b);
    
    // Apply strength to create the tint effect
    // The overlay is semi-transparent with the warm color
    float alpha = strength * 0.25f;  // Max 25% opacity for subtle effect
    
    // Tint the color (reduce blue, enhance red/orange)
    float tint_r = r * strength;
    float tint_g = g * strength * 0.7f;  // Reduce green slightly
    float tint_b = b * strength * 0.3f;  // Significantly reduce blue
    
    LOG_INFO_FMT("Applying night light: temp={:.0f}K, RGB=({:.2f},{:.2f},{:.2f}), "
                 "tinted=({:.2f},{:.2f},{:.2f}), alpha={:.2f}",
                 config_.temperature, r, g, b, tint_r, tint_g, tint_b, alpha);
    
    // Update rectangle color
    wlr_scene_rect_set_color(overlay_rect_, 
                             (float[4]){tint_r, tint_g, tint_b, alpha});
    
    // Enable overlay
    wlr_scene_node_set_enabled(&overlay_rect_->node, true);
    
    // Ensure it's on top
    wlr_scene_node_raise_to_top(&overlay_rect_->node);
    
    LOG_INFO("Night light overlay enabled and raised to top");
}

void NightLight::TemperatureToRGB(float temperature, float& r, float& g, float& b) {
    // Convert color temperature (in Kelvin) to RGB
    // Based on Tanner Helland's algorithm
    // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
    
    float temp = temperature / 100.0f;
    
    // Calculate red
    if (temp <= 66.0f) {
        r = 1.0f;
    } else {
        float red = temp - 60.0f;
        red = 329.698727446f * std::pow(red, -0.1332047592f);
        r = std::max(0.0f, std::min(1.0f, red / 255.0f));
    }
    
    // Calculate green
    if (temp <= 66.0f) {
        float green = 99.4708025861f * std::log(temp) - 161.1195681661f;
        g = std::max(0.0f, std::min(1.0f, green / 255.0f));
    } else {
        float green = temp - 60.0f;
        green = 288.1221695283f * std::pow(green, -0.0755148492f);
        g = std::max(0.0f, std::min(1.0f, green / 255.0f));
    }
    
    // Calculate blue
    if (temp >= 66.0f) {
        b = 1.0f;
    } else if (temp <= 19.0f) {
        b = 0.0f;
    } else {
        float blue = temp - 10.0f;
        blue = 138.5177312231f * std::log(blue) - 305.0447927307f;
        b = std::max(0.0f, std::min(1.0f, blue / 255.0f));
    }
}

} // namespace Leviathan
