#ifndef BLE_SIMPLE_SERVER_DEVICE_INCLUDED
#define BLE_SIMPLE_SERVER_DEVICE_INCLUDED

#include "ble_server_obj.h" 
#include "ble_advertising.h" 

#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleServerDevice
{
public:
    static void init(std::string deviceName);
    static BLESimpleServer* createServer();
    static BLESimpleAdvertising* getAdvertising();
    static void startAdvertising();
    static void stopAdvertising();

    static void gattServerEventHandler(esp_gatts_cb_event_t event,
    esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

private:
    static BLESimpleAdvertising* m_bleAdvertising;
    static BLESimpleServer* m_pServer;
    static uint16_t m_appId;
    static bool m_initialized;

private:
    static bool btStart();
};

#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_SERVER_DEVICE_INCLUDED
