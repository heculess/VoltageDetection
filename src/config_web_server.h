#ifndef DEBUG_WEB_SERVER_INCLUDED
#define DEBUG_WEB_SERVER_INCLUDED

#include <functional>
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WBSRV_CMD_HANDLER_CALLBACK std::function<bool(httpd_req_t *,char*,char*,char*)>

class ConfigWebServer
{
public:
    ConfigWebServer();
    virtual ~ConfigWebServer();

    bool start_server();
    void stop_server();
    
    void loop();

    bool get_server_status();
    void set_server_status(bool running);

    void set_cmd_handler_callback(WBSRV_CMD_HANDLER_CALLBACK callback);

private:
    httpd_handle_t config_httpd;

    WBSRV_CMD_HANDLER_CALLBACK _cmd_callback_handler;
    static void restart_later(void *pvParameters);
private:
    static esp_err_t config_handler(httpd_req_t *req);
    static esp_err_t index_handler(httpd_req_t *req);
};


#ifdef __cplusplus
}
#endif


#endif // DEBUG_WEB_SERVER_INCLUDED
