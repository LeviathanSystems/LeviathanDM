# Simple Logger Library

## Overview

A lightweight, thread-safe logging library for LeviathanDM that avoids external dependencies like spdlog/fmt. This eliminates symbol resolution issues with plugins.

## Features

- **Thread-safe**: Uses a queue and background thread for async file writing
- **Zero external dependencies**: No spdlog, no fmt, just standard C++
- **Colored console output**: Different colors for different log levels
- **File logging**: Writes to `leviathan.log` asynchronously
- **Simple formatting**: Basic `{}` placeholder formatting support
- **Header-only**: Easy to integrate into any project

## Usage

### Basic Logging

```cpp
#include "logger/SimpleLogger.hpp"

// Initialize (usually in main)
Leviathan::SimpleLogger::Instance().Init("leviathan.log", Leviathan::LogLevel::DEBUG);

// Simple logging
LOG_INFO("Application started");
LOG_ERROR("Something went wrong");
```

### Formatted Logging

```cpp
#include "logger/SimpleLogger.hpp"
#include "logger/LogFormat.hpp"

LOG_INFO_FMT("Loaded plugin: {} v{}", name, version);
LOG_ERROR_FMT("Failed to open file: {}", filename);
LOG_DEBUG_FMT("Processing {} items in {} seconds", count, duration);
```

## Log Levels

- `TRACE`: Detailed trace information
- `DEBUG`: Debug information
- `INFO`: Informational messages
- `WARN`: Warning messages
- `ERROR`: Error messages  
- `CRITICAL`: Critical errors

## Integration

### In Main Compositor

```cmake
# In CMakeLists.txt
add_subdirectory(logger)
target_link_libraries(leviathan-ui INTERFACE leviathan-logger)
```

### In Plugins

Plugins automatically get logging through `libleviathan-ui.so` - no separate linking needed!

## Migration from spdlog

Replace:
```cpp
#include <spdlog/spdlog.h>
LOG_INFO("Message: {}", value);
```

With:
```cpp
#include "logger/SimpleLogger.hpp"
#include "logger/LogFormat.hpp"
LOG_INFO_FMT("Message: {}", value);
```

## Benefits

1. **No symbol conflicts**: Each component sees the same header-only implementation
2. **Fast compilation**: No template-heavy library to compile
3. **Simple debugging**: Easy to understand and modify
4. **Plugin-safe**: Works perfectly across shared library boundaries
5. **Automatic versioning**: Plugins show version in logs automatically
