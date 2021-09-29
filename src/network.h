#ifndef _NETWORK_H
#define _NETWORK_H

#include "WiFi.h"

#ifdef __cplusplus
extern "C"
{
#endif
    namespace Network
    {
        void esp_initialize_sntp(void);
        bool auto_connect_wifi(int max_try = 10);
        int get_wifi_status();

    }
#ifdef __cplusplus
}
#endif

#endif // UPLOADER_INCLUDED
