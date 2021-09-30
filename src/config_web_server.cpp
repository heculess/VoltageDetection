#include "config_web_server.h"
#include "device_core.h"
#include "psram_buffer.h"
#include <string>


ConfigWebServer::ConfigWebServer(): 
config_httpd(NULL),
_cmd_callback_handler(NULL)
{
    start_server();
}

ConfigWebServer::~ConfigWebServer()
{
}

bool ConfigWebServer::start_server()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_POST,
        .handler = config_handler,
        .user_ctx = this};

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = this};

    if (httpd_start(&config_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(config_httpd, &config_uri);
        httpd_register_uri_handler(config_httpd, &index_uri);
    }

    printf("start config web server\r\n");

    return true;
}

void ConfigWebServer::stop_server()
{
    httpd_stop(config_httpd);
    printf("debug web server stopped\r\n");
}

void ConfigWebServer::set_cmd_handler_callback(WBSRV_CMD_HANDLER_CALLBACK callback)
{
    _cmd_callback_handler = callback;
}

void ConfigWebServer::loop()
{
    if (!get_server_status())
        stop_server();
}

esp_err_t ConfigWebServer::config_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_url_query_len(req);
    if (buf_len < 1)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    buf_len += 1;
    PsramBuffer cmd_buf(buf_len);
    if (!cmd_buf.is_valid())
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, cmd_buf.as_char_ptr(), cmd_buf.length()) != ESP_OK)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    PsramBuffer value(buf_len);
    bool config_change = false;

    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "broker", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        if(DeviceCore::get_mqtt_broker() != value.as_char_ptr()){
            DeviceCore::set_mqtt_broker(value.as_char_ptr());
            config_change = true;
        }    
    }

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "username", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        if(DeviceCore::get_mqtt_username() != value.as_char_ptr()){
            DeviceCore::set_mqtt_username(value.as_char_ptr());
            config_change = true;
        }
    }

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "password", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        if(DeviceCore::get_mqtt_password() != value.as_char_ptr()){
            DeviceCore::set_mqtt_password(value.as_char_ptr());
            config_change = true;
        }
    }

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "topic", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        if(DeviceCore::get_mqtt_topic() != value.as_char_ptr())
        {
            DeviceCore::set_mqtt_topic(value.as_char_ptr());
            config_change = true;
        }
    }

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "devicename", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        if(DeviceCore::get_device_name() != value.as_char_ptr())
        {
            DeviceCore::set_device_name(value.as_char_ptr());
            config_change = true;
        }
    }

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "port", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        int cfg_port = atoi(value.as_char_ptr());
        if(DeviceCore::get_mqtt_port() != cfg_port)
        {
            DeviceCore::set_mqtt_port(cfg_port);
            config_change = true;
        }
    }

    httpd_resp_set_type(req, "text/html");

    std::string response = std::string("Config succeed !!");
    if(config_change){
        response.append("\r\nDevice will restart! Please wait for a few minutes ...");
    }

    esp_err_t resp_code = httpd_resp_send(req, response.c_str(), response.length());
    if(config_change)
        DeviceCore::GotoDeepSleepAndExit(5);

    return resp_code;
}

esp_err_t ConfigWebServer::index_handler(httpd_req_t *req)
{
    printf("index_handler\r\n");
    httpd_resp_set_type(req, "text/html");

    std::string response = std::string("<!DOCTYPE html><html><head>"
        "<META http-equiv=Content-Type content=\"text/html; charset=GB2312\">"
        "<style>.div-left{ float:left;width:30%;height:25px;text-align:left;}"
        ".div-right{ float:right;width:70%;height:25px;text-align:right;}"
        "</style></head><body><div style=\"width:100%;text-align:center\">"
        "<h2>MQTT 设置</h2><form action=\"#\" method=\"post\">"
        "<div style=\"width:500px;margin:0px auto;\">"
        "<div class=\"div-left\">MQTT 服务器地址：</div><div class=\"div-right\">"
        "<input style=\"width:330px\" name=\"broker\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_broker());
        response.append("\"></div><div class=\"div-left\">MQTT 服务器端口：</div>"
            "<div class=\"div-right\"><input name=\"port\" type=\"text\" value=\"");
        response.append(std::to_string(DeviceCore::get_mqtt_port()));
        response.append("\"></div><div class=\"div-left\">MQTT 客户端ID：</div><div class=\"div-right\">"
            "<input name=\"devicename\" type=\"text\" value=\"");
        response.append(DeviceCore::get_device_name());
        response.append("\"></div><div class=\"div-left\">MQTT 用户名：</div><div class=\"div-right\">"
            "<input name=\"username\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_username());
        response.append("\"></div><div class=\"div-left\">MQTT 密码：</div><div class=\"div-right\">"
            "<input name=\"password\" type=\"password\" value=\"");
        response.append(DeviceCore::get_mqtt_password());
        response.append("\"></div><div class=\"div-left\">MQTT 订阅地址：</div><div class=\"div-right\">"
            "<input name=\"topic\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_topic());
        response.append("\"></div><div style=\"float:left;width:100%;height:30px;\">"
            "</div><div style=\"height:25px;text-align:right;\"><input type=\"submit\" value=\"提交\"></div>"
            "</div></form></div></body></html>");

    return httpd_resp_send(req, response.c_str(), response.length());
}