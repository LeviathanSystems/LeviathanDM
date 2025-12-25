#!/bin/bash

# Test script for LeviathanDM notification daemon
# This tests the org.freedesktop.Notifications implementation

echo "=== LeviathanDM Notification Daemon Test ==="
echo ""

# Check if notification daemon is running
if ! busctl --user list | grep -q "org.freedesktop.Notifications"; then
    echo "❌ Notification daemon not running!"
    echo "   Make sure LeviathanDM is running."
    exit 1
fi

echo "✅ Notification daemon is running"
echo ""

# Test 1: Simple notification
echo "Test 1: Simple notification"
notify-send "LeviathanDM" "Notification daemon is working!"
sleep 2

# Test 2: Notification with body
echo "Test 2: Notification with body text"
notify-send "Test Title" "This is the notification body text. It can be longer and span multiple lines if needed."
sleep 2

# Test 3: Urgency levels
echo "Test 3: Low urgency"
notify-send -u low "Low Priority" "This is a low priority notification"
sleep 1

echo "Test 4: Normal urgency"
notify-send -u normal "Normal Priority" "This is a normal priority notification"
sleep 1

echo "Test 5: Critical urgency"
notify-send -u critical "Critical Alert" "This is a critical notification!"
sleep 2

# Test 4: With icon
echo "Test 6: Notification with icon"
notify-send -i battery-low "Battery Warning" "Battery level is low (10%)"
sleep 2

# Test 5: Multiple notifications
echo "Test 7: Multiple notifications"
notify-send "First" "First notification"
notify-send "Second" "Second notification"
notify-send "Third" "Third notification"
sleep 3

# Test 6: Long timeout
echo "Test 8: Notification with custom timeout (10 seconds)"
notify-send -t 10000 "Long Timeout" "This notification will stay for 10 seconds"
sleep 2

# Test 7: Persistent notification (doesn't expire)
echo "Test 9: Persistent notification"
notify-send -t 0 "Persistent" "This notification won't expire automatically"
sleep 2

# Test 8: Get capabilities
echo ""
echo "=== Notification Daemon Capabilities ==="
busctl --user call org.freedesktop.Notifications /org/freedesktop/Notifications org.freedesktop.Notifications GetCapabilities

# Test 9: Get server information
echo ""
echo "=== Notification Daemon Server Information ==="
busctl --user call org.freedesktop.Notifications /org/freedesktop/Notifications org.freedesktop.Notifications GetServerInformation

echo ""
echo "=== Tests Complete ==="
echo ""
echo "The notification daemon supports:"
echo "  • Standard desktop notifications via notify-send"
echo "  • Urgency levels (low, normal, critical)"
echo "  • Icons"
echo "  • Custom timeouts"
echo "  • Persistent notifications"
echo "  • Multiple simultaneous notifications"
echo ""
echo "Unlike AwesomeWM, LeviathanDM has a built-in notification daemon!"
