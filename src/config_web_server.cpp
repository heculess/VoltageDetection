#include "config_web_server.h"
#include "device_core.h"
#include "psram_buffer.h"
#include <string>

unsigned char FromHex(unsigned char x)   
{   
    unsigned char y;  
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;  
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;  
    else if (x >= '0' && x <= '9') y = x - '0';  
    else assert(0);  
    return y;  
} 

std::string UrlDecode(const std::string& str)  
{  
    std::string strTemp = "";  
    size_t length = str.length();  
    for (size_t i = 0; i < length; i++)  
    {  
        if (str[i] == '+') strTemp += ' ';  
        else if (str[i] == '%')  
        {  
            assert(i + 2 < length);  
            unsigned char high = FromHex((unsigned char)str[++i]);  
            unsigned char low = FromHex((unsigned char)str[++i]);  
            strTemp += high*16 + low;  
        }  
        else strTemp += str[i];  
    }  
    return strTemp;  
}


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
        printf("start config web server\r\n");
        return true;
    }

    return false;
}

void ConfigWebServer::stop_server()
{
    httpd_stop(config_httpd);
    printf("config web server stopped\r\n");
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
    size_t buf_len = req->content_len;
    if (buf_len < 1)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    PsramBuffer cmd_buf(buf_len+1);
    httpd_req_recv(req, cmd_buf.as_char_ptr(), buf_len);
  
    if (!cmd_buf.is_valid())
    {
        httpd_resp_send_500(req);
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
        std::string topic = UrlDecode(value.as_char_ptr());
        if(DeviceCore::get_mqtt_topic() != topic)
        {
            DeviceCore::set_mqtt_topic(topic.c_str());
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

    value.zero_buffer();
    if (httpd_query_key_value(cmd_buf.as_char_ptr(), "loopdelay", value.as_char_ptr(), value.length()) == ESP_OK)
    {
        int loop_delay = atoi(value.as_char_ptr());
        if(DeviceCore::get_mainloop_delay() != loop_delay)
        {
            DeviceCore::set_mainloop_delay(loop_delay);
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
        xTaskCreate(restart_later, "restart_later", 1024, NULL,
                configMAX_PRIORITIES - 5, NULL);
    
    return resp_code;
}

void ConfigWebServer::restart_later(void *pvParameters)
{
    vTaskDelay(500 / portTICK_RATE_MS);
    DeviceCore::GotoDeepSleepAndExit(5);
    vTaskDelete(NULL);
}

esp_err_t ConfigWebServer::index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");

    std::string response = std::string("<!DOCTYPE html><html><head>"
        "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">"
        "<style>.div-left{ float:left;width:30%;height:25px;text-align:left;}"
        ".div-right{ float:right;width:70%;height:25px;text-align:right;}"
        "</style></head><body><div style=\"width:100%;text-align:center\">"
        "<h2>MQTT 设置</h2><form action=\"/config\" method=\"post\">"
        "<div style=\"width:500px;margin:0px auto;\">"
        "<div class=\"div-left\">MQTT 服务器地址：</div><div class=\"div-right\">"
        "<input style=\"width:330px\" name=\"broker\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_broker());
        response.append("\"></div><div class=\"div-left\">MQTT 服务器端口：</div>"
            "<div class=\"div-right\"><input style=\"width:330px\" name=\"port\" type=\"text\" value=\"");
        response.append(std::to_string(DeviceCore::get_mqtt_port()));
        response.append("\"></div><div class=\"div-left\">MQTT 客户端ID：</div><div class=\"div-right\">"
            "<input style=\"width:330px\" name=\"devicename\" type=\"text\" value=\"");
        response.append(DeviceCore::get_device_name());
        response.append("\"></div><div class=\"div-left\">MQTT 用户名：</div><div class=\"div-right\">"
            "<input style=\"width:330px\" style=\"width:330px\" name=\"username\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_username());
        response.append("\"></div><div class=\"div-left\">MQTT 密码：</div><div class=\"div-right\">"
            "<input style=\"width:330px\" name=\"password\" type=\"password\" value=\"");
        response.append(DeviceCore::get_mqtt_password());
        response.append("\"></div><div class=\"div-left\">MQTT 订阅地址：</div><div class=\"div-right\">"
            "<input style=\"width:330px\" name=\"topic\" type=\"text\" value=\"");
        response.append(DeviceCore::get_mqtt_topic());
        response.append("\"></div><div class=\"div-left\">主循环延迟间隔：</div><div class=\"div-right\">"
            "<input style=\"width:330px\" name=\"loopdelay\" type=\"text\" value=\"");
        response.append(std::to_string(DeviceCore::get_mainloop_delay()));
        response.append("\"></div><div style=\"float:left;width:100%;height:30px;\">"
            "</div><div style=\"height:25px;text-align:right;\"><input type=\"submit\" value=\"提交\"></div>"
            "</div></form></div></body></html>");

    return httpd_resp_send(req, response.c_str(), response.length());
}