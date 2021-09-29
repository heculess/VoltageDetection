#include "HTTPClient.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <string.h>
#include "psram_buffer.h"

static const char *TAG = "HTTPClient";

#define DEFAULT_HTTP_PORT (80)
#define DEFAULT_HTTPS_PORT (443)

HttpOnDataCallback::HttpOnDataCallback() : _data_buffer(NULL)
{
}

HttpOnDataCallback::~HttpOnDataCallback()
{
    clear_data_buffer();
}

void HttpOnDataCallback::onRequestBegin()
{
    clear_data_buffer();
}

bool HttpOnDataCallback::onDataHeader(char *header_key, char *header_value)
{
    return true;
}

bool HttpOnDataCallback::onDataBody(size_t data_len, uint8_t *data_buffer)
{
    int last_data_len = 0;
    if (!_data_buffer)
    {
        _data_buffer = new PsramBuffer(data_len);
    }
    else
    {
        PsramBuffer *_last_data = _data_buffer;
        last_data_len = _last_data->length();
        _data_buffer = new PsramBuffer(last_data_len + data_len);
        memcpy(_data_buffer->ptr(), _last_data->ptr(), last_data_len);
        delete _last_data;
        _last_data = NULL;
    }

    memcpy(_data_buffer->ptr() + last_data_len, data_buffer, data_len);
    return true;
}

void HttpOnDataCallback::clear_data_buffer()
{
    if (_data_buffer)
    {
        delete _data_buffer;
        _data_buffer = NULL;
    }
}

uint8_t *HttpOnDataCallback::get_buffer_ptr()
{
    if (_data_buffer)
    {
        return _data_buffer->ptr();
    }
    return NULL;
}

int HttpOnDataCallback::get_buffer_size()
{
    if (_data_buffer)
    {
        return _data_buffer->length();
    }
    return 0;
}

std::string HttpOnDataCallback::to_string()
{
    return std::string((char *)get_buffer_ptr(), get_buffer_size());
}

esp_err_t HTTPClient::_http_event_handle(esp_http_client_event_t *evt)
{
    HTTPClient *_client = (HTTPClient *)evt->user_data;
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADERS_SENT:
    {
        if (_client)
        {
            _client->clear_output_buffer();
        }
    }
    break;
    case HTTP_EVENT_ON_HEADER:
    {
        if (_client)
        {
            bool http_continue = true;
            if (_client->_on_data_callback)
            {
                http_continue = _client->_on_data_callback->onDataHeader(
                    evt->header_key, evt->header_value);
            }
            if (_client->_response_header.find(evt->header_key) != _client->_response_header.end())
                _client->_response_header[evt->header_key] = evt->header_value;

            if (!http_continue)
                esp_http_client_close(evt->client);
        }
    }
    break;
    case HTTP_EVENT_ON_DATA:
    {
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        bool http_continue = true;
        if (_client && _client->_on_data_callback)
        {
            http_continue = _client->_on_data_callback->onDataBody(evt->data_len, (uint8_t *)evt->data);
        }
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (_client->_output_buffer)
            {
                _client->clear_output_buffer();
            }

            _client->_output_buffer = new PsramBuffer(evt->data_len);
            memcpy(_client->_output_buffer->ptr(), evt->data, evt->data_len);
        }
        if (!http_continue)
            esp_http_client_close(evt->client);
    }
    break;
    case HTTP_EVENT_ON_FINISH:
    {
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (_client && _client->_on_data_callback)
        {
            _client->_on_data_callback->onFinish();
        }
    }
    break;
    case HTTP_EVENT_DISCONNECTED:
    {
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        if (_client && _client->_on_data_callback)
        {
            _client->_on_data_callback->onDisconnect();
        }
    }
    break;
    }
    return ESP_OK;
}

HTTPClient::HTTPClient(const char *host, uint16_t port, const char *ssl_cert, int timeout_ms) : _client_handle(NULL),
                                                                                                _output_buffer(NULL),
                                                                                                _on_data_callback(NULL)
{
    init_client(host, port, ssl_cert, timeout_ms);
}

HTTPClient::HTTPClient(const char *url, const char *ssl_cert, int timeout_ms) : _client_handle(NULL),
                                                                                _output_buffer(NULL),
                                                                                _on_data_callback(NULL)
{
    struct http_parser_url purl;
    http_parser_url_init(&purl);

    if (http_parser_parse_url(url, strlen(url), 0, &purl) != 0)
    {
        ESP_LOGE(TAG, "Error parse url %s", url);
        return;
    }

    std::string host;
    if (purl.field_data[UF_HOST].len)
    {
        host = std::string(url + purl.field_data[UF_HOST].off, purl.field_data[UF_HOST].len);
    }

    uint16_t port = DEFAULT_HTTP_PORT;
    if (purl.field_data[UF_SCHEMA].len)
    {
        std::string schema = std::string(url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
        if (schema == "https")
        {
            port = DEFAULT_HTTPS_PORT;
        }
    }
    if (purl.field_data[UF_PORT].len)
    {
        port = strtol((const char *)(url + purl.field_data[UF_PORT].off), NULL, 10);
    }

    init_client(host.c_str(), port, ssl_cert, timeout_ms);
}

HTTPClient::~HTTPClient()
{
    if (_client_handle)
    {
        esp_http_client_cleanup(_client_handle);
        _client_handle = NULL;
    }

    clear_output_buffer();
}

void HTTPClient::init_client(const char *host, uint16_t port, const char *ssl_cert, int timeout_ms)
{
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));

    config.buffer_size_tx = 1024;
    config.buffer_size = 1024;
    config.host = host;
    config.port = port;
    config.path = "/";
    config.event_handler = _http_event_handle,
    config.timeout_ms = timeout_ms;
    config.user_data = this;

    if (ssl_cert)
    {
        config.cert_pem = ssl_cert;
        config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    }
    _client_handle = esp_http_client_init(&config);
}

void HTTPClient::clear_output_buffer()
{
    if (_output_buffer)
    {
        delete _output_buffer;
        _output_buffer = NULL;
    }
}

void HTTPClient::set_header(const char *key, const char *value)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return;
    }
    esp_err_t err = esp_http_client_set_header(_client_handle, key, value);
    if (err == ESP_FAIL)
        ESP_LOGE(TAG, "set_header failed");
}

void HTTPClient::delete_header(const char *key)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return;
    }
    esp_http_client_delete_header(_client_handle, key);
}

void HTTPClient::set_url(const char *path)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return;
    }
    esp_http_client_set_url(_client_handle, path);
}

int HTTPClient::GET(const char *url)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return -1;
    }
    clear_output_buffer();

    if (_on_data_callback)
        _on_data_callback->onRequestBegin();

    if (url)
        esp_http_client_set_url(_client_handle, url);

    esp_http_client_set_method(_client_handle, HTTP_METHOD_GET);
    esp_http_client_perform(_client_handle);

    return esp_http_client_get_status_code(_client_handle);
}

int HTTPClient::POST(const char *url, uint8_t *payload, size_t size)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return -1;
    }

    clear_output_buffer();

    if (_on_data_callback)
        _on_data_callback->onRequestBegin();

    if (url)
        esp_http_client_set_url(_client_handle, url);

    esp_http_client_set_method(_client_handle, HTTP_METHOD_POST);
    esp_http_client_set_post_field(_client_handle, (const char *)payload, size);
    esp_http_client_perform(_client_handle);
    return esp_http_client_get_status_code(_client_handle);
}

int HTTPClient::PUT(const char *url, const uint8_t *payload, size_t size)
{
    if (_client_handle == NULL)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return -1;
    }

    clear_output_buffer();

    if (_on_data_callback)
        _on_data_callback->onRequestBegin();

    if (url)
        esp_http_client_set_url(_client_handle, url);

    esp_http_client_set_method(_client_handle, HTTP_METHOD_PUT);

    esp_http_client_set_post_field(_client_handle, (const char *)payload, size);

    esp_http_client_perform(_client_handle);

    return esp_http_client_get_status_code(_client_handle);
}

void HTTPClient::set_chunk_callback(HttpOnDataCallback *data_callback)
{
    _on_data_callback = data_callback;
}

int HTTPClient::getSize(void)
{
    if (!_client_handle)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return false;
    }

    return esp_http_client_get_content_length(_client_handle);
}

int HTTPClient::read_data(uint8_t *buffer, size_t size)
{
    if (!_client_handle)
    {
        ESP_LOGE(TAG, "HTTP Handle is null");
        return false;
    }

    return esp_http_client_read(_client_handle, (char *)buffer, size);
}

void HTTPClient::collect_headers(const char *headerKeys[], const size_t headerKeysCount)
{
    _response_header.clear();
    for (size_t i = 0; i < headerKeysCount; i++)
        _response_header.insert(std::make_pair(std::string(headerKeys[i]), std::string("")));
}

std::string HTTPClient::get_response_header()
{
    std::string headers;
    for (std::map<std::string, std::string>::iterator it = _response_header.begin();
         it != _response_header.end(); it++)
    {
        headers.append(it->first + ":" + it->second + "\r\n");
    }

    return headers;
}

std::string HTTPClient::get_header(const char *name)
{
    std::map<std::string, std::string>::iterator it = _response_header.find(name);
    if (it != _response_header.end())
        return it->second;
    return std::string("");
}

std::string HTTPClient::get_response()
{
    if (!_output_buffer)
        return std::string();

    return std::string(_output_buffer->as_char_ptr(), _output_buffer->length());
}