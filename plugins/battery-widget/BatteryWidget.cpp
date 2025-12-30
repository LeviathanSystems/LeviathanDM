#include "../../include/ui/PeriodicWidget.hpp"
#include "../../include/ui/DBusHelper.hpp"
#include "../../include/ui/IPopoverProvider.hpp"
#include "../../include/ui/reusable-widgets/Popover.hpp"
#include "../../include/ui/Widget.hpp"
#include "../../include/ui/reusable-widgets/Label.hpp"
#include "../../include/Logger.hpp"
#include "version.h"
#include <gio/gio.h>
#include <vector>
#include <map>

namespace Leviathan {
namespace UI {

/**
 * @brief Battery device information from UPower
 */
struct BatteryDevice {
    std::string path;           // DBus object path
    std::string native_path;    // Kernel device path
    std::string model;          // Device model name
    std::string vendor;         // Device vendor/manufacturer
    std::string serial;         // Serial number
    uint32_t type;              // Device type (battery, mouse, keyboard, etc.)
    double percentage;          // Battery percentage (0-100)
    int64_t time_to_empty;     // Seconds until empty
    int64_t time_to_full;      // Seconds until full
    double energy;              // Current energy in Wh
    double energy_full;         // Maximum energy in Wh
    double energy_rate;         // Power draw in W
    double voltage;             // Voltage in V
    uint32_t state;             // Charging state
    bool is_present;            // Device is present
    bool is_rechargeable;       // Device has rechargeable battery
    std::string icon_name;      // Icon name for display
    
    // Type enum values (from UPower)
    static constexpr uint32_t TYPE_UNKNOWN = 0;
    static constexpr uint32_t TYPE_LINE_POWER = 1;
    static constexpr uint32_t TYPE_BATTERY = 2;
    static constexpr uint32_t TYPE_UPS = 3;
    static constexpr uint32_t TYPE_MONITOR = 4;
    static constexpr uint32_t TYPE_MOUSE = 5;
    static constexpr uint32_t TYPE_KEYBOARD = 6;
    static constexpr uint32_t TYPE_PDA = 7;
    static constexpr uint32_t TYPE_PHONE = 8;
    static constexpr uint32_t TYPE_MEDIA_PLAYER = 9;
    static constexpr uint32_t TYPE_TABLET = 10;
    static constexpr uint32_t TYPE_COMPUTER = 11;
    static constexpr uint32_t TYPE_GAMING_INPUT = 12;
    static constexpr uint32_t TYPE_PEN = 13;
    static constexpr uint32_t TYPE_TOUCHPAD = 14;
    static constexpr uint32_t TYPE_MODEM = 15;
    static constexpr uint32_t TYPE_NETWORK = 16;
    static constexpr uint32_t TYPE_HEADSET = 17;
    static constexpr uint32_t TYPE_SPEAKERS = 18;
    static constexpr uint32_t TYPE_HEADPHONES = 19;
    static constexpr uint32_t TYPE_VIDEO = 20;
    static constexpr uint32_t TYPE_OTHER_AUDIO = 21;
    static constexpr uint32_t TYPE_REMOTE_CONTROL = 22;
    static constexpr uint32_t TYPE_PRINTER = 23;
    static constexpr uint32_t TYPE_SCANNER = 24;
    static constexpr uint32_t TYPE_CAMERA = 25;
    static constexpr uint32_t TYPE_WEARABLE = 26;
    static constexpr uint32_t TYPE_TOY = 27;
    static constexpr uint32_t TYPE_BLUETOOTH_GENERIC = 28;
    
    // State enum values
    static constexpr uint32_t STATE_UNKNOWN = 0;
    static constexpr uint32_t STATE_CHARGING = 1;
    static constexpr uint32_t STATE_DISCHARGING = 2;
    static constexpr uint32_t STATE_EMPTY = 3;
    static constexpr uint32_t STATE_FULLY_CHARGED = 4;
    static constexpr uint32_t STATE_PENDING_CHARGE = 5;
    static constexpr uint32_t STATE_PENDING_DISCHARGE = 6;
    
    std::string GetTypeString() const {
        switch (type) {
            case TYPE_BATTERY: return "Battery";
            case TYPE_MOUSE: return "Mouse";
            case TYPE_KEYBOARD: return "Keyboard";
            case TYPE_HEADSET: return "Headset";
            case TYPE_HEADPHONES: return "Headphones";
            case TYPE_GAMING_INPUT: return "Controller";
            case TYPE_PHONE: return "Phone";
            case TYPE_TABLET: return "Tablet";
            case TYPE_PEN: return "Pen";
            case TYPE_TOUCHPAD: return "Touchpad";
            case TYPE_SPEAKERS: return "Speakers";
            case TYPE_WEARABLE: return "Wearable";
            default: return "Device";
        }
    }
    
    std::string GetStateString() const {
        switch (state) {
            case STATE_CHARGING: return "Charging";
            case STATE_DISCHARGING: return "Discharging";
            case STATE_FULLY_CHARGED: return "Full";
            case STATE_EMPTY: return "Empty";
            case STATE_PENDING_CHARGE: return "Pending";
            case STATE_PENDING_DISCHARGE: return "Pending";
            default: return "Unknown";
        }
    }
    
    std::string GetIcon() const {
        if (!is_present) return "󰂃";  // Battery empty/missing
        
        if (type == TYPE_MOUSE) return "󰍽";
        if (type == TYPE_KEYBOARD) return "󰌌";
        if (type == TYPE_HEADSET || type == TYPE_HEADPHONES) return "󰋋";
        if (type == TYPE_GAMING_INPUT) return "󰖺";
        if (type == TYPE_PHONE) return "󰄜";
        
        // Battery icons based on percentage
        if (state == STATE_CHARGING) {
            if (percentage >= 90) return "󰂅";
            if (percentage >= 70) return "󰂋";
            if (percentage >= 50) return "󰂉";
            if (percentage >= 30) return "󰂇";
            if (percentage >= 10) return "󰢜";
            return "󰢟";
        }
        
        if (percentage >= 90) return "󰁹";
        if (percentage >= 80) return "󰂂";
        if (percentage >= 70) return "󰂁";
        if (percentage >= 60) return "󰂀";
        if (percentage >= 50) return "󰁿";
        if (percentage >= 40) return "󰁾";
        if (percentage >= 30) return "󰁽";
        if (percentage >= 20) return "󰁼";
        if (percentage >= 10) return "󰁻";
        return "󰁺";
    }
};

/**
 * @brief Battery widget with UPower integration
 * 
 * Shows main battery status and provides a popover with all battery-powered devices.
 * Monitors:
 * - Laptop battery (percentage, charging state, time remaining)
 * - Bluetooth devices (headsets, mice, keyboards, controllers)
 * - Other UPS/battery devices
 * 
 * Configuration options:
 * - update_interval: How often to poll battery status (default: 30s)
 * - show_percentage: Show battery percentage text (default: true)
 * - show_time: Show time remaining (default: false)
 * - low_battery_threshold: Percentage to warn at (default: 20)
 * - critical_battery_threshold: Percentage for critical warning (default: 10)
 */
class BatteryWidget : public PeriodicWidget, public DBusHelper, public UI::IPopoverProvider {
public:
    BatteryWidget() 
        : show_percentage_(true),
          show_time_(false),
          low_threshold_(20.0),
          critical_threshold_(10.0),
          main_battery_percentage_(0.0),
          main_battery_state_(BatteryDevice::STATE_UNKNOWN),
          on_ac_power_(false) {
        
        // Create popover for showing all devices
        popover_ = std::make_shared<Popover>();
    }
    
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            .name = PLUGIN_NAME,
            .version = PLUGIN_VERSION,
            .author = "LeviathanDM",
            .description = "Battery status with UPower integration - shows main battery and connected devices",
            .api_version = WIDGET_API_VERSION
        };
    }
    
    // IPopoverProvider implementation
    std::shared_ptr<UI::Popover> GetPopover() const override {
        return popover_;
    }
    
    bool HasPopover() const override {
        return popover_ != nullptr;
    }

protected:
    bool InitializeImpl(const std::map<std::string, std::string>& config) override {
        // Parse widget-specific config
        auto show_pct_it = config.find("show_percentage");
        if (show_pct_it != config.end()) {
            show_percentage_ = (show_pct_it->second == "true" || show_pct_it->second == "1");
        }
        
        auto show_time_it = config.find("show_time");
        if (show_time_it != config.end()) {
            show_time_ = (show_time_it->second == "true" || show_time_it->second == "1");
        }
        
        auto low_it = config.find("low_battery_threshold");
        if (low_it != config.end()) {
            low_threshold_ = std::stod(low_it->second);
        }
        
        auto crit_it = config.find("critical_battery_threshold");
        if (crit_it != config.end()) {
            critical_threshold_ = std::stod(crit_it->second);
        }
        
        // Connect to UPower via DBus
        if (!ConnectToSystemBus()) {
            LOG_ERROR("BatteryWidget: Failed to connect to system bus");
            return false;
        }
        
        // Create proxy for UPower main interface
        if (!CreateProxy("org.freedesktop.UPower",
                        "/org/freedesktop/UPower",
                        "org.freedesktop.UPower")) {
            LOG_ERROR("BatteryWidget: Failed to create UPower proxy");
            return false;
        }
        
        // Subscribe to device added/removed signals
        SubscribeToSignal("DeviceAdded", [this](const std::string& signal, GVariant* params) {
            UpdateBatteryInfo();
        });
        
        SubscribeToSignal("DeviceRemoved", [this](const std::string& signal, GVariant* params) {
            UpdateBatteryInfo();
        });
        
        // Defer initial update to avoid blocking during initialization
        // The first UpdateData() call will populate the battery info
        dirty_ = true;
        
        LOG_INFO_FMT("BatteryWidget v{} initialized successfully", PLUGIN_VERSION);
        return true;
    }
    
    void UpdateData() override {
        UpdateBatteryInfo();
    }
    
    void CalculateSize(int available_width, int available_height) override {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        // Build display string
        std::string display_text = BuildDisplayText();
        
        int text_width, text_height;
        MeasureText(display_text, text_width, text_height, 8);
        
        width_ = std::min(text_width, available_width);
        height_ = std::min(text_height, available_height);
    }
    
    void Render(cairo_t* cr) override {
        if (!IsVisible()) return;
        
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        cairo_save(cr);
        
        // Choose color based on battery level
        double r = text_color_[0];
        double g = text_color_[1];
        double b = text_color_[2];
        
        if (main_battery_percentage_ <= critical_threshold_) {
            r = 1.0; g = 0.0; b = 0.0;  // Red for critical
        } else if (main_battery_percentage_ <= low_threshold_) {
            r = 1.0; g = 0.5; b = 0.0;  // Orange for low
        }
        
        std::string display_text = BuildDisplayText();
        double center_x = x_ + width_ / 2.0;
        double center_y = y_ + height_ / 2.0;
        
        // Use custom color
        cairo_set_source_rgba(cr, r, g, b, text_color_[3]);
        cairo_select_font_face(cr, font_family_.c_str(),
                             CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, display_text.c_str(), &extents);
        
        cairo_move_to(cr,
                     center_x - extents.width / 2 - extents.x_bearing,
                     center_y - extents.height / 2 - extents.y_bearing);
        cairo_show_text(cr, display_text.c_str());
        
        cairo_restore(cr);
    }
    
    // Override click handler to manage popover
    bool HandleClick(int click_x, int click_y) override {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (click_x >= x_ && click_x <= x_ + width_ &&
            click_y >= y_ && click_y <= y_ + height_) {
            
            // Toggle popover
            if (popover_->IsVisible()) {
                popover_->Hide();
            } else {
                // Update popover content
                UpdatePopover();
                
                // Position popover below widget
                popover_->SetPosition(x_, y_ + height_ + 2);
                popover_->CalculateSize();
                popover_->Show();
            }
            
            RequestRender();
            return true;
        }
        return false;
    }

private:
    void UpdateBatteryInfo() {
        // Enumerate all devices - this is the most reliable way
        GVariant* result = CallMethod("EnumerateDevices");
        if (result) {
            devices_.clear();
            main_battery_percentage_ = 0.0;
            main_battery_state_ = BatteryDevice::STATE_UNKNOWN;
            bool found_main_battery = false;
            
            GVariantIter iter;
            const char* device_path;
            
            g_variant_iter_init(&iter, g_variant_get_child_value(result, 0));
            while (g_variant_iter_next(&iter, "&o", &device_path)) {
                BatteryDevice device = QueryDeviceInfo(device_path);
                
                // First battery device is typically the main one
                if (!found_main_battery && device.type == BatteryDevice::TYPE_BATTERY) {
                    main_battery_percentage_ = device.percentage;
                    main_battery_state_ = device.state;
                    found_main_battery = true;
                }
                
                // Store all devices for popover
                if (device.is_present && device.percentage > 0) {
                    devices_.push_back(device);
                }
            }
            
            g_variant_unref(result);
            dirty_ = true;
        }
        
        // Check AC power status
        GVariant* on_battery = GetProperty("OnBattery");
        if (on_battery) {
            on_ac_power_ = !GetBoolFromVariant(on_battery);
            g_variant_unref(on_battery);
        }
    }
    
    BatteryDevice QueryDeviceInfo(const std::string& device_path) {
        BatteryDevice device;
        device.path = device_path;
        
        // Create temporary proxy for this device
        GError* error = nullptr;
        GDBusProxy* device_proxy = g_dbus_proxy_new_sync(
            GetConnection(),
            G_DBUS_PROXY_FLAGS_NONE,
            nullptr,
            "org.freedesktop.UPower",
            device_path.c_str(),
            "org.freedesktop.UPower.Device",
            nullptr,
            &error
        );
        
        if (error) {
            LOG_ERROR_FMT("Failed to create device proxy: {}", error->message);
            g_error_free(error);
            return device;
        }
        
        // Get all properties
        auto get_prop = [&](const char* name) -> GVariant* {
            return g_dbus_proxy_get_cached_property(device_proxy, name);
        };
        
        GVariant* val;
        
        if ((val = get_prop("NativePath"))) {
            device.native_path = GetStringFromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("Model"))) {
            device.model = GetStringFromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("Vendor"))) {
            device.vendor = GetStringFromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("Type"))) {
            device.type = GetUInt32FromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("Percentage"))) {
            device.percentage = GetDoubleFromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("State"))) {
            device.state = GetUInt32FromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("TimeToEmpty"))) {
            device.time_to_empty = GetInt64FromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("TimeToFull"))) {
            device.time_to_full = GetInt64FromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("IsPresent"))) {
            device.is_present = GetBoolFromVariant(val);
            g_variant_unref(val);
        }
        
        if ((val = get_prop("IsRechargeable"))) {
            device.is_rechargeable = GetBoolFromVariant(val);
            g_variant_unref(val);
        }
        
        g_object_unref(device_proxy);
        
        return device;
    }
    
    std::string BuildDisplayText() const {
        std::string text;
        
        // Get icon for current state
        BatteryDevice temp;
        temp.percentage = main_battery_percentage_;
        temp.state = main_battery_state_;
        temp.is_present = true;
        temp.type = BatteryDevice::TYPE_BATTERY;
        
        text = temp.GetIcon();
        
        if (show_percentage_) {
            text += " " + std::to_string(static_cast<int>(main_battery_percentage_)) + "%";
        }
        
        return text;
    }
    
    void UpdatePopover() {
        if (!popover_) return;
        
        LOG_DEBUG("UpdatePopover: Clearing old content");
        // Clear old content first
        popover_->ClearContent();
        
        LOG_DEBUG_FMT("UpdatePopover: Creating new content with {} devices", devices_.size());
        // Create a VBox to hold all device rows
        auto container = std::make_shared<UI::VBox>();
        container->SetSpacing(4);
        
        // Add main battery row
        auto main_row = std::make_shared<UI::HBox>();
        main_row->SetSpacing(8);
        
        auto main_icon = std::make_shared<UI::Label>("󰁹");
        main_icon->SetFontSize(14);
        
        auto main_text = std::make_shared<UI::Label>("Main Battery");
        main_text->SetFontSize(12);
        
        auto main_percentage = std::make_shared<UI::Label>(
            std::to_string(static_cast<int>(main_battery_percentage_)) + "%"
        );
        main_percentage->SetFontSize(12);
        main_percentage->SetTextColor(0.7, 0.7, 0.7, 1.0);
        
        main_row->AddChild(main_icon);
        main_row->AddChild(main_text);
        main_row->AddChild(main_percentage);
        
        container->AddChild(main_row);
        
        // Add a separator if there are other devices
        if (devices_.size() > 1) {
            auto separator = std::make_shared<UI::Label>("────────────────");
            separator->SetFontSize(8);
            separator->SetTextColor(0.4, 0.4, 0.4, 1.0);
            container->AddChild(separator);
        }
        
        // Add all other devices (skip the first battery which is the main one)
        bool first_battery = true;
        for (const auto& device : devices_) {
            // Skip the main battery (first battery device)
            if (device.type == BatteryDevice::TYPE_BATTERY && first_battery) {
                first_battery = false;
                continue;
            }
            
            auto device_row = std::make_shared<UI::HBox>();
            device_row->SetSpacing(8);
            
            auto device_icon = std::make_shared<UI::Label>(device.GetIcon());
            device_icon->SetFontSize(14);
            
            std::string device_name;
            if (!device.model.empty()) {
                device_name = device.model;
            } else if (!device.vendor.empty()) {
                device_name = device.vendor + " " + device.GetTypeString();
            } else {
                device_name = device.GetTypeString();
            }
            
            auto device_text = std::make_shared<UI::Label>(device_name);
            device_text->SetFontSize(12);
            
            auto device_percentage = std::make_shared<UI::Label>(
                std::to_string(static_cast<int>(device.percentage)) + "%"
            );
            device_percentage->SetFontSize(12);
            device_percentage->SetTextColor(0.7, 0.7, 0.7, 1.0);
            
            device_row->AddChild(device_icon);
            device_row->AddChild(device_text);
            device_row->AddChild(device_percentage);
            
            container->AddChild(device_row);
        }
        
        // If no other devices, show a message
        if (devices_.empty()) {
            auto empty_label = std::make_shared<UI::Label>("No other devices");
            empty_label->SetFontSize(12);
            empty_label->SetTextColor(0.5, 0.5, 0.5, 1.0);
            container->AddChild(empty_label);
        }
        
        // Set the container as the popover content
        popover_->SetContent(container);
        LOG_DEBUG("UpdatePopover: Content set, calculating size");
        popover_->CalculateSize();
        int pw, ph;
        popover_->GetSize(pw, ph);
        LOG_DEBUG_FMT("UpdatePopover: Popover size calculated: {}x{}", pw, ph);
    }

    // Configuration
    bool show_percentage_;
    bool show_time_;
    double low_threshold_;
    double critical_threshold_;
    
    // Battery state
    double main_battery_percentage_;
    uint32_t main_battery_state_;
    bool on_ac_power_;
    
    // All battery devices (Bluetooth peripherals, etc.)
    std::vector<BatteryDevice> devices_;
    
    // Popover
    std::shared_ptr<Popover> popover_;
};

} // namespace UI
} // namespace Leviathan

// Export plugin functions
extern "C" {
    EXPORT_PLUGIN_CREATE(BatteryWidget)
    EXPORT_PLUGIN_DESTROY(BatteryWidget)
    EXPORT_PLUGIN_METADATA(BatteryWidget)
}
