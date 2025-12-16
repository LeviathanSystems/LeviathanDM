#include "wayland/Server.hpp"
#include "Logger.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    // Initialize logger
    Leviathan::Logger::Init();
    
    LOG_INFO("LeviathanDM - Wayland Compositor");
    LOG_INFO("Starting compositor...");
    
    auto server = Leviathan::Wayland::Server::Create();
    if (!server) {
        LOG_ERROR("Failed to initialize compositor");
        return EXIT_FAILURE;
    }
    
    server->Run();
    
    delete server;
    return EXIT_SUCCESS;
}
