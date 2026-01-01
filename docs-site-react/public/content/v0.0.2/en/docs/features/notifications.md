---
title: "Notification Daemon"
weight: 1
---

# Notification Daemon

LeviathanDM includes a **built-in notification daemon** that implements the [Desktop Notifications Specification v1.2](https://specifications.freedesktop.org/notification-spec/latest/).

{{< hint info >}}
**Unlike AwesomeWM**, you don't need external daemons like dunst or notification-daemon!
{{< /hint >}}

## Features

✅ Full Desktop Notifications Spec v1.2  
✅ DBus Integration (`org.freedesktop.Notifications`)  
✅ Multiple simultaneous notifications  
✅ Urgency levels (low, normal, critical)  
✅ Custom timeouts and persistent notifications  
✅ Action support (buttons)  
✅ Icon support  
✅ Pango markup in body text  

## Usage

### Command Line

```bash
# Simple notification
notify-send "Hello" "World"

# With icon
notify-send -i battery-low "Battery Warning" "10% remaining"

# Urgency levels
notify-send -u low "Low Priority"
notify-send -u normal "Normal"
notify-send -u critical "CRITICAL ALERT"

# Custom timeout (milliseconds)
notify-send -t 10000 "Long Timeout" "Stays for 10 seconds"

# Persistent (doesn't expire)
notify-send -t 0 "Persistent" "Won't auto-close"
```

### From Applications

Any application using libnotify or the Desktop Notifications spec works automatically:

#### Python
```python
import notify2
notify2.init("MyApp")
n = notify2.Notification("Title", "Message", "icon-name")
n.show()
```

#### Node.js
```javascript
const notifier = require('node-notifier');
notifier.notify({
  title: 'Title',
  message: 'Message'
});
```

#### Rust
```rust
use notify_rust::Notification;
Notification::new()
    .summary("Title")
    .body("Message")
    .show()?;
```

## DBus Interface

**Bus Name:** `org.freedesktop.Notifications`  
**Object Path:** `/org/freedesktop/Notifications`  
**Interface:** `org.freedesktop.Notifications`

### Methods

#### GetCapabilities() → as
Returns supported capabilities:
- `body`, `body-markup`, `body-hyperlinks`
- `icon-static`, `actions`, `action-icons`
- `persistence`

#### Notify(susssasa{sv}i) → u
Display a notification.

**Parameters:**
- `s` app_name
- `u` replaces_id (0 for new)
- `s` app_icon
- `s` summary (title)
- `s` body
- `as` actions
- `a{sv}` hints
- `i` expire_timeout (-1=default, 0=never)

**Returns:** Notification ID

#### CloseNotification(u)
Close notification by ID.

#### GetServerInformation() → (ssss)
Returns: name, vendor, version, spec_version

### Signals

#### NotificationClosed(uu)
Emitted when closed.
- `u` id
- `u` reason (1=expired, 2=dismissed, 3=called, 4=undefined)

#### ActionInvoked(us)
Emitted when action clicked.
- `u` id
- `s` action_key

## Testing

Test the notification daemon:

```bash
./test-notifications.sh
```

Or manually:

```bash
# Check if running
busctl --user list | grep Notifications

# Get capabilities
busctl --user call org.freedesktop.Notifications \
  /org/freedesktop/Notifications \
  org.freedesktop.Notifications \
  GetCapabilities

# Send test notification
notify-send "Test" "Hello from LeviathanDM"
```

## Visual Rendering

{{< hint warning >}}
**Status:** Protocol implementation complete, visual rendering planned for future release.
{{< /hint >}}

Currently notifications are:
- ✅ Received and stored
- ✅ Expire automatically
- ✅ Emit DBus signals
- ⏳ Not yet rendered visually

Future rendering will include:
- Wayland layer surfaces
- Cairo rendering with title, body, icons
- Top-right corner positioning
- Vertical stacking
- Fade in/out animations
- Click handling for actions

## Comparison

| Feature | LeviathanDM | AwesomeWM | i3/Sway |
|---------|-------------|-----------|---------|
| Built-in daemon | ✅ Yes | ❌ No | ❌ No |
| External daemon needed | ❌ No | ✅ dunst/notify-daemon | ✅ mako/dunst |
| DBus interface | ✅ Full spec | N/A | N/A |
| Customization | 🚧 Planned | ✅ Lua | ✅ Config files |

## See Also

- [Technical Documentation](https://github.com/LeviathanSystems/LeviathanDM/blob/master/docs/NOTIFICATION_DAEMON.md)
- [Implementation Summary](https://github.com/LeviathanSystems/LeviathanDM/blob/master/docs/NOTIFICATION_IMPLEMENTATION_SUMMARY.md)
