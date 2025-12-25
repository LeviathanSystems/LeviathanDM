#pragma once

#include <wlr/types/wlr_buffer.h>
#include <cstddef>
#include <cstdint>

namespace Leviathan {

/**
 * Custom SHM buffer implementation that manually initializes wlr_buffer
 * without relying on the unexported wlr_buffer_init() function.
 * 
 * This is a workaround for the fact that wlroots doesn't export buffer
 * initialization functions for compositor-side buffers.
 */
class ShmBuffer {
public:
    /**
     * Create an SHM buffer with the specified dimensions.
     * 
     * @param width Buffer width in pixels
     * @param height Buffer height in pixels
     * @return Pointer to created buffer, or nullptr on failure
     */
    static ShmBuffer* Create(int width, int height);
    
    /**
     * Get the underlying wlr_buffer pointer.
     */
    wlr_buffer* GetWlrBuffer() { return &buffer_; }
    
    /**
     * Get pointer to the pixel data for rendering.
     */
    void* GetData() { return data_; }
    
    /**
     * Get the stride (bytes per row).
     */
    size_t GetStride() const { return stride_; }
    
    /**
     * Get the buffer size in bytes.
     */
    size_t GetSize() const { return size_; }

private:
    ShmBuffer(int width, int height, int fd, void* data, size_t size);
    ~ShmBuffer();
    
    // Disable copy/move
    ShmBuffer(const ShmBuffer&) = delete;
    ShmBuffer& operator=(const ShmBuffer&) = delete;
    
    // wlr_buffer callbacks
    static void BufferDestroy(wlr_buffer* wlr_buffer);
    static bool BufferGetShm(wlr_buffer* wlr_buffer, wlr_shm_attributes* attribs);
    static bool BufferBeginDataPtrAccess(wlr_buffer* wlr_buffer, uint32_t flags,
                                          void** data, uint32_t* format, size_t* stride);
    static void BufferEndDataPtrAccess(wlr_buffer* wlr_buffer);
    
    // Create a memfd for shared memory
    static int CreateShmFd(size_t size);
    
    // Buffer implementation vtable
    static const wlr_buffer_impl buffer_impl_;
    
    // Members
    wlr_buffer buffer_;      // Must be first for pointer casting
    int fd_;
    void* data_;
    size_t size_;
    size_t stride_;
    bool data_ptr_locked_;
};

} // namespace Leviathan
