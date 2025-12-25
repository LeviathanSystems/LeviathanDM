#include "ui/ShmBuffer.hpp"
#include "Logger.hpp"

#include <wlr/interfaces/wlr_buffer.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <drm_fourcc.h>

namespace Leviathan {

// Static buffer implementation vtable
const wlr_buffer_impl ShmBuffer::buffer_impl_ = {
    .destroy = ShmBuffer::BufferDestroy,
    .get_dmabuf = nullptr,  // We don't support DMA-BUF
    .get_shm = ShmBuffer::BufferGetShm,
    .begin_data_ptr_access = ShmBuffer::BufferBeginDataPtrAccess,
    .end_data_ptr_access = ShmBuffer::BufferEndDataPtrAccess,
};

int ShmBuffer::CreateShmFd(size_t size) {
    int fd = memfd_create("leviathan-statusbar", MFD_CLOEXEC);
    if (fd < 0) {
        LOG_ERROR("Failed to create memfd");
        return -1;
    }
    
    if (ftruncate(fd, size) < 0) {
        LOG_ERROR_FMT("Failed to resize memfd to {} bytes", size);
        close(fd);
        return -1;
    }
    
    return fd;
}

ShmBuffer* ShmBuffer::Create(int width, int height) {
    if (width <= 0 || height <= 0) {
        LOG_ERROR_FMT("Invalid buffer dimensions: {}x{}", width, height);
        return nullptr;
    }
    
    // Calculate buffer size (ARGB8888 = 4 bytes per pixel)
    size_t stride = width * 4;
    size_t size = stride * height;
    
    // Create shared memory file descriptor
    int fd = CreateShmFd(size);
    if (fd < 0) {
        return nullptr;
    }
    
    // Map the shared memory
    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        LOG_ERROR("Failed to mmap SHM buffer");
        close(fd);
        return nullptr;
    }
    
    // Create the buffer object
    ShmBuffer* buffer = new ShmBuffer(width, height, fd, data, size);
    
    LOG_INFO_FMT("Created ShmBuffer: {}x{}, fd={}, size={} bytes", 
             width, height, fd, size);
    
    return buffer;
}

ShmBuffer::ShmBuffer(int width, int height, int fd, void* data, size_t size)
    : fd_(fd)
    , data_(data)
    , size_(size)
    , stride_(width * 4)
    , data_ptr_locked_(false)
{
    // Manually initialize the wlr_buffer structure
    // This is what wlr_buffer_init() would do, but it's not exported
    
    buffer_.impl = &buffer_impl_;
    buffer_.width = width;
    buffer_.height = height;
    buffer_.dropped = false;
    buffer_.n_locks = 1;  // Start with 1 reference (the creator)
    buffer_.accessing_data_ptr = false;
    
    // Initialize signals
    wl_signal_init(&buffer_.events.destroy);
    wl_signal_init(&buffer_.events.release);
    
    // Initialize addon set manually (since wlr_addon_set_init is not exported)
    // The addon_set contains a private wl_list that we need to initialize
    // We access it via pointer arithmetic since it's the only member
    wl_list* addon_list = reinterpret_cast<wl_list*>(&buffer_.addons);
    wl_list_init(addon_list);
    
    LOG_DEBUG_FMT("Manually initialized wlr_buffer struct at {}", 
              static_cast<void*>(&buffer_));
}

ShmBuffer::~ShmBuffer() {
    LOG_DEBUG_FMT("Destroying ShmBuffer (fd={})", fd_);
    
    // Unmap memory
    if (data_ && data_ != MAP_FAILED) {
        munmap(data_, size_);
        data_ = nullptr;
    }
    
    // Close file descriptor
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    
    // No need to call wlr_addon_set_finish since we manually initialized it
    // and we don't expect any addons to be attached to our buffer
}

void ShmBuffer::BufferDestroy(wlr_buffer* wlr_buffer) {
    // Cast back to ShmBuffer (buffer_ is first member, so address matches)
    ShmBuffer* buffer = reinterpret_cast<ShmBuffer*>(wlr_buffer);
    
    LOG_DEBUG_FMT("BufferDestroy callback for ShmBuffer at {}", 
              static_cast<void*>(buffer));
    
    // Emit destroy signal (wlroots convention)
    wl_signal_emit(&wlr_buffer->events.destroy, nullptr);
    
    // Delete the buffer object
    delete buffer;
}

bool ShmBuffer::BufferGetShm(wlr_buffer* wlr_buffer, wlr_shm_attributes* attribs) {
    ShmBuffer* buffer = reinterpret_cast<ShmBuffer*>(wlr_buffer);
    
    attribs->fd = buffer->fd_;
    attribs->format = DRM_FORMAT_ARGB8888;
    attribs->width = wlr_buffer->width;
    attribs->height = wlr_buffer->height;
    attribs->stride = buffer->stride_;
    attribs->offset = 0;
    
    LOG_TRACE_FMT("BufferGetShm: fd={}, {}x{}, stride={}", 
              attribs->fd, attribs->width, attribs->height, attribs->stride);
    
    return true;
}

bool ShmBuffer::BufferBeginDataPtrAccess(wlr_buffer* wlr_buffer, uint32_t /* flags */,
                                         void** data, uint32_t* format, size_t* stride) {
    ShmBuffer* buffer = reinterpret_cast<ShmBuffer*>(wlr_buffer);
    
    if (buffer->data_ptr_locked_) {
        LOG_WARN("Buffer data pointer already locked");
        return false;
    }
    
    *data = buffer->data_;
    *format = DRM_FORMAT_ARGB8888;
    *stride = buffer->stride_;
    
    buffer->data_ptr_locked_ = true;
    wlr_buffer->accessing_data_ptr = true;
    
    LOG_TRACE_FMT("BufferBeginDataPtrAccess: data={}, format=ARGB8888, stride={}", 
              *data, *stride);
    
    return true;
}

void ShmBuffer::BufferEndDataPtrAccess(wlr_buffer* wlr_buffer) {
    ShmBuffer* buffer = reinterpret_cast<ShmBuffer*>(wlr_buffer);
    
    if (!buffer->data_ptr_locked_) {
        LOG_WARN("Buffer data pointer was not locked");
        return;
    }
    
    buffer->data_ptr_locked_ = false;
    wlr_buffer->accessing_data_ptr = false;
    
    LOG_TRACE("BufferEndDataPtrAccess");
}

} // namespace Leviathan
