#ifndef HTTPClient_H_
#define HTTPClient_H_

#include <memory>
#include <map>
#include <string>
#include <esp_http_client.h>
#include "psram_buffer.h"

typedef enum {
    HTTP_CODE_CONTINUE = 100,
    HTTP_CODE_SWITCHING_PROTOCOLS = 101,
    HTTP_CODE_PROCESSING = 102,
    HTTP_CODE_OK = 200,
    HTTP_CODE_CREATED = 201,
    HTTP_CODE_ACCEPTED = 202,
    HTTP_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_CODE_NO_CONTENT = 204,
    HTTP_CODE_RESET_CONTENT = 205,
    HTTP_CODE_PARTIAL_CONTENT = 206,
    HTTP_CODE_MULTI_STATUS = 207,
    HTTP_CODE_ALREADY_REPORTED = 208,
    HTTP_CODE_IM_USED = 226,
    HTTP_CODE_MULTIPLE_CHOICES = 300,
    HTTP_CODE_MOVED_PERMANENTLY = 301,
    HTTP_CODE_FOUND = 302,
    HTTP_CODE_SEE_OTHER = 303,
    HTTP_CODE_NOT_MODIFIED = 304,
    HTTP_CODE_USE_PROXY = 305,
    HTTP_CODE_TEMPORARY_REDIRECT = 307,
    HTTP_CODE_PERMANENT_REDIRECT = 308,
    HTTP_CODE_BAD_REQUEST = 400,
    HTTP_CODE_UNAUTHORIZED = 401,
    HTTP_CODE_PAYMENT_REQUIRED = 402,
    HTTP_CODE_FORBIDDEN = 403,
    HTTP_CODE_NOT_FOUND = 404,
    HTTP_CODE_METHOD_NOT_ALLOWED = 405,
    HTTP_CODE_NOT_ACCEPTABLE = 406,
    HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_CODE_REQUEST_TIMEOUT = 408,
    HTTP_CODE_CONFLICT = 409,
    HTTP_CODE_GONE = 410,
    HTTP_CODE_LENGTH_REQUIRED = 411,
    HTTP_CODE_PRECONDITION_FAILED = 412,
    HTTP_CODE_PAYLOAD_TOO_LARGE = 413,
    HTTP_CODE_URI_TOO_LONG = 414,
    HTTP_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_CODE_RANGE_NOT_SATISFIABLE = 416,
    HTTP_CODE_EXPECTATION_FAILED = 417,
    HTTP_CODE_MISDIRECTED_REQUEST = 421,
    HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
    HTTP_CODE_LOCKED = 423,
    HTTP_CODE_FAILED_DEPENDENCY = 424,
    HTTP_CODE_UPGRADE_REQUIRED = 426,
    HTTP_CODE_PRECONDITION_REQUIRED = 428,
    HTTP_CODE_TOO_MANY_REQUESTS = 429,
    HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
    HTTP_CODE_NOT_IMPLEMENTED = 501,
    HTTP_CODE_BAD_GATEWAY = 502,
    HTTP_CODE_SERVICE_UNAVAILABLE = 503,
    HTTP_CODE_GATEWAY_TIMEOUT = 504,
    HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_CODE_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_CODE_INSUFFICIENT_STORAGE = 507,
    HTTP_CODE_LOOP_DETECTED = 508,
    HTTP_CODE_NOT_EXTENDED = 510,
    HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
} t_http_codes;

class HttpOnDataCallback {
public:
    HttpOnDataCallback();
	virtual ~HttpOnDataCallback();
    virtual void onRequestBegin();
    virtual bool onDataHeader(char *header_key, char *header_value);
    virtual bool onDataBody(size_t size, uint8_t * data_buffer);

    virtual void onFinish(){}
    virtual void onDisconnect(){}

    uint8_t * get_buffer_ptr();
    int get_buffer_size();

    std::string to_string();

protected:
    PsramBuffer * _data_buffer;

protected:
    void clear_data_buffer();
};

class HTTPClient
{
public:
    HTTPClient(const char * host, uint16_t port = 80, const char * ssl_cert = NULL, int timeout_ms = 0);
    HTTPClient(const char * url, const char * ssl_cert = NULL, int timeout_ms = 0);
    virtual ~HTTPClient();

    void set_header(const char * key, const char * value);
    void delete_header(const char * key);
    void set_url(const char * path);

    int GET(const char * url);
    int POST(const char * url, uint8_t * payload, size_t size);
    int PUT(const char * url, const uint8_t * payload, size_t size);

    void set_chunk_callback(HttpOnDataCallback* data_callback);


    int getSize(void);

    int read_data(uint8_t * buffer, size_t size);

    void collect_headers(const char* headerKeys[], const size_t headerKeysCount);
    std::string get_response();
    std::string get_response_header();
    std::string get_header(const char * name);

private:
    std::map<std::string, std::string> _response_header;
    esp_http_client_handle_t _client_handle;
    PsramBuffer * _output_buffer;
    HttpOnDataCallback* _on_data_callback;

    static esp_err_t _http_event_handle(esp_http_client_event_t *evt);
    void clear_output_buffer();

    void init_client(const char *host, uint16_t port, const char *ssl_cert, int timeout_ms);
};



#endif /* HTTPClient_H_ */
