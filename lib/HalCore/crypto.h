#ifndef CRYPTO_INCLUDED
#define CRYPTO_INCLUDED

#include "mbedtls/md5.h"
#include "esp32/rom/md5_hash.h"
#include "mbedtls/sha1.h"
#include "mbedtls/md.h"
#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

    class MD5Builder
    {
    public:
        MD5Builder();
        virtual ~MD5Builder();

        void update(const uint8_t *data, size_t length);
        std::string calculate();

    private:
        MD5Context _md5_ctx;
    };

    class Sha1Builder
    {
    public:
        Sha1Builder();
        virtual ~Sha1Builder();

        void update(const uint8_t *data, size_t length);
        std::string calculate();

    private:
        mbedtls_sha1_context _sha1_ctx;
    };

    class HmacSha1Builder
    {
    public:
        HmacSha1Builder();
        virtual ~HmacSha1Builder();

        void setup(const uint8_t *secret, size_t secret_length);

        void update(const uint8_t *data, size_t length);
        void calculate(uint8_t * buffer);
        std::string calculate();
        

    private:
        mbedtls_md_context_t _md_ctx;
    };

    std::string toMD5(uint8_t *data, size_t length);
    std::string to_base64(const uint8_t * data, size_t length);
    std::string HMAC_SHA1_to_base64(const uint8_t *secret, size_t secretLength, const std::string &content);


#ifdef __cplusplus
}
#endif

#endif // CRYPTO_INCLUDED
