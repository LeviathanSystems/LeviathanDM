#!/bin/bash
# Start compositor with logging
cage ./build/leviathan 2>&1 | tee /tmp/leviathan.log
