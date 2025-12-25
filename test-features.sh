#!/usr/bin/env bash
# Test script for clipboard and plugin stats

echo "═══════════════════════════════════════════════════"
echo " LeviathanDM Feature Test Script"
echo "═══════════════════════════════════════════════════"
echo

# Test 1: Plugin Stats
echo "━━━ Test 1: Plugin Memory Statistics ━━━"
echo
echo "Running: leviathanctl get-plugin-stats"
echo
./build/leviathanctl get-plugin-stats
echo
echo "Expected: Should show BatteryWidget and ClockWidget with memory usage"
echo

# Test 2: Clipboard (requires compositor running)
echo "━━━ Test 2: Clipboard Support ━━━"
echo
echo "The clipboard error should be GONE from the compositor logs."
echo "Look for: 'Clipboard and primary selection support enabled'"
echo
echo "To test clipboard manually:"
echo "  1. Start compositor: cage ./build/leviathan"
echo "  2. Open alacritty terminal"
echo "  3. Test commands:"
echo "     echo 'Hello World' | wl-copy"
echo "     wl-paste"
echo "  4. Or use Ctrl+C/V in any GUI app"
echo

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " Checking compositor logs..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -f "leviathan.log" ]; then
    echo
    echo "Last 20 lines of leviathan.log:"
    tail -20 leviathan.log | grep --color=always -E "Clipboard|plugin|Plugin|IPC.*PLUGIN|error 65544|$"
else
    echo "No leviathan.log file found (compositor not running yet)"
fi

echo
echo "═══════════════════════════════════════════════════"
echo " Test Complete!"
echo "═══════════════════════════════════════════════════"
