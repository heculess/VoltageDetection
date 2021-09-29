#include "psram_buffer.h"
#include <esp_heap_caps.h>

PsramBuffer::PsramBuffer(size_t length) :
_length(length),
_buffer(length>0?
#ifdef CONFIG_ESP32_SPIRAM_SUPPORT
heap_caps_calloc(length, 1, MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)
#else
heap_caps_calloc(length, 1,  MALLOC_CAP_8BIT)
#endif
:NULL)
{
}

PsramBuffer::~PsramBuffer()
{
    if (_buffer)
    {
        heap_caps_free(_buffer);
        _buffer = NULL;
    }
}

bool PsramBuffer::is_valid()
{
    if (_buffer)
        return true;
    return false;
}

uint8_t *PsramBuffer::ptr()
{
    return (uint8_t *)_buffer;
}

char *PsramBuffer::as_char_ptr()
{
    return (char *)_buffer;
}

size_t PsramBuffer::length()
{
    return _length;
}