#include "ui/menubar/providers/BookmarksProvider.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <unistd.h>

namespace Leviathan {
namespace UI {

// ============================================================================
// BookmarkMenuItem
// ============================================================================

BookmarkMenuItem::BookmarkMenuItem(const std::string& name,
                                   const std::string& target,
                                   BookmarkType type,
                                   const std::string& description)
    : name_(name)
    , target_(target)
    , type_(type)
    , description_(description)
{
}

std::vector<std::string> BookmarkMenuItem::GetSearchKeywords() const {
    return {name_, target_, description_};
}

void BookmarkMenuItem::Execute() {
    std::string command;
    
    switch (type_) {
        case BookmarkType::Directory:
            // Open file manager in directory
            command = "xdg-open \"" + target_ + "\"";
            break;
        case BookmarkType::File:
            // Open file with default application
            command = "xdg-open \"" + target_ + "\"";
            break;
        case BookmarkType::URL:
            // Open URL in browser
            command = "xdg-open \"" + target_ + "\"";
            break;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        exit(1);
    } else if (pid < 0) {
        LOG_ERROR_FMT("Failed to open bookmark: {}", name_);
    } else {
        LOG_INFO_FMT("Opened bookmark: {}", name_);
    }
}

// ============================================================================
// BookmarkProvider
// ============================================================================

BookmarkProvider::BookmarkProvider() {
}

std::vector<std::shared_ptr<MenuItem>> BookmarkProvider::LoadItems() {
    return bookmarks_;
}

void BookmarkProvider::Refresh() {
    // Bookmarks are managed directly, no refresh needed
}

void BookmarkProvider::AddBookmark(const std::string& name,
                                  const std::string& target,
                                  BookmarkMenuItem::BookmarkType type,
                                  const std::string& description)
{
    bookmarks_.push_back(std::make_shared<BookmarkMenuItem>(
        name, target, type, description));
}

void BookmarkProvider::ClearBookmarks() {
    bookmarks_.clear();
}

} // namespace UI
} // namespace Leviathan
