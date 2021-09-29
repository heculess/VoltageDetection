#include "network.h"
#include <esp_wifi.h>
#include "lwip/apps/sntp.h"
#include "string.h"

void Network::esp_initialize_sntp(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *)("ntp1.aliyun.com"));
    sntp_setservername(2, (char *)("cn.ntp.org.cn"));
    sntp_init();
}

int Network::get_wifi_status()
{
    int wstatus = WiFi.status();
    if (wstatus == WL_CONNECTED)
    {
        printf(" WIFI Success\r\n");
        printf("SSID: %s , PSW: %s\r\n", WiFi.SSID().c_str(), WiFi.psk().c_str());
        printf("IP: %s/%s\r\n", WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
    }
    return wstatus;
}

bool Network::auto_connect_wifi(int max_try)
{
    for (int i = 0; i < max_try; i++)
    {
        if (get_wifi_status() == WL_CONNECTED)
        {
            printf("\r\n");
            return true;
        }
        else
        {
            printf("%d", i);
            fflush(stdout);
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }
    printf(" WIFI Faild!\r\n");
    return false;
}