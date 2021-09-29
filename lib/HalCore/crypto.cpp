#include "crypto.h"
#include "psram_buffer.h"
#include <algorithm>
#include <mbedtls/base64.h>
#include <esp_log.h>

static const char *TAG = "crypto";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// MD5Builder
///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MD5Builder::MD5Builder()
{
    MD5Init(&_md5_ctx);
}

MD5Builder::~MD5Builder()
{
}

void MD5Builder::update(const uint8_t *data, size_t length)
{
    MD5Update(&_md5_ctx, data, length);
}

std::string MD5Builder::calculate()
{
    unsigned char decrypt[16] = {0};
    MD5Final(decrypt,&_md5_ctx);

    char md5_out[33];
    for(uint8_t i = 0; i < 16; i++) {
        sprintf(md5_out + (i * 2), "%02x", decrypt[i]);
    }

    return std::string(md5_out);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Sha1Builder
///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sha1Builder::Sha1Builder()
{
    mbedtls_sha1_init(&_sha1_ctx);
    mbedtls_sha1_starts(&_sha1_ctx);
}

Sha1Builder::~Sha1Builder()
{
    mbedtls_sha1_free(&_sha1_ctx);
}

void Sha1Builder::update(const uint8_t *data, size_t length)
{
    mbedtls_sha1_update(&_sha1_ctx, data, length);
}

std::string Sha1Builder::calculate()
{
    unsigned char decrypt[20] = {0};
    mbedtls_sha1_finish(&_sha1_ctx, decrypt);

    char sha1_out[41];
    for(uint8_t i = 0; i < 20; i++) {
        sprintf(sha1_out + (i * 2), "%02x", decrypt[i]);
    }

    return std::string(sha1_out);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// HmacSha1Builder
///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HmacSha1Builder::HmacSha1Builder()
{
    mbedtls_md_init(&_md_ctx);
    mbedtls_md_setup(&_md_ctx,mbedtls_md_info_from_type(MBEDTLS_MD_SHA1),1);
}

HmacSha1Builder::~HmacSha1Builder()
{
    mbedtls_md_free(&_md_ctx);
}

void HmacSha1Builder::setup(const uint8_t *secret, size_t secret_length)
{
    mbedtls_md_hmac_starts(&_md_ctx, secret, secret_length);
}

void HmacSha1Builder::update(const uint8_t *data, size_t length)
{
    mbedtls_md_hmac_update(&_md_ctx, data, length);
}

void HmacSha1Builder::calculate(uint8_t * buffer)
{
    mbedtls_md_hmac_finish(&_md_ctx, buffer);
}

std::string HmacSha1Builder::calculate()
{
    unsigned char decrypt[32] = {0};
    calculate(decrypt);

    char sha1_out[65];
    for(uint8_t i = 0; i < 20; i++) {
        sprintf(sha1_out + (i * 2), "%02x", decrypt[i]);
    }

    return std::string(sha1_out);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string UUID()
{
    time_t rawtime;
    time(&rawtime);
    std::string time = std::to_string(rawtime);
    return toMD5((uint8_t *)time.c_str(), time.length());
}

std::string toMD5(uint8_t *data, size_t length)
{
    MD5Builder md5_ctx;
    size_t wb_count = 0;
    do
    {
        size_t toRead = std::min(length - wb_count, (size_t)2048);
        md5_ctx.update(data + wb_count, toRead);
        wb_count += toRead;

    } while (wb_count < length);
    
    return md5_ctx.calculate();
}

std::string to_base64(const uint8_t * data, size_t length)
{
    PsramBuffer base64_buf(length * 4);
    if (!base64_buf.is_valid()) {
        ESP_LOGE(TAG, "malloc for base64 buffer failed");
        return "";
    }

    size_t out_len = 0;
    mbedtls_base64_encode(base64_buf.ptr(), length * 4, &out_len, data, length);
    return std::string(base64_buf.as_char_ptr());
}

std::string HMAC_SHA1_to_base64(const uint8_t *secret, size_t secretLength, const std::string &content)
{
    HmacSha1Builder md_ctx;
    md_ctx.setup(secret,secretLength);
    md_ctx.update((uint8_t *)content.c_str(),content.length());

    uint8_t decrypt[32] = {0};
    md_ctx.calculate(decrypt);
    return to_base64(decrypt, 20);
}

