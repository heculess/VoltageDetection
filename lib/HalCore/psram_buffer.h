#ifndef PSRAM_INCLUDED
#define PSRAM_INCLUDED

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

class PsramBuffer
{
public:
    PsramBuffer(size_t length);
    virtual ~PsramBuffer();

    bool is_valid();
    uint8_t *ptr();
    char *as_char_ptr();
    size_t length();

private:
    const size_t _length;
    void *_buffer;
};  

#ifdef __cplusplus
}
#endif

#endif // PSRAM_INCLUDED