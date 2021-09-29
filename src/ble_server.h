#ifndef CUSTOM_BLE_SERVER_COM_INCLUDED
#define CUSTOM_BLE_SERVER_COM_INCLUDED
#include "ble_server_device.h"

#ifdef __cplusplus
extern "C" {
#endif


class BLEComServer
{
public:
    BLEComServer();
    virtual ~BLEComServer();

    class ConnectEvtCallbacks: public BLESimpleServerCallbacks
    {
    public:
        ConnectEvtCallbacks(BLEComServer& server);
        void onConnect(BLESimpleServer* pServer);
        void onDisconnect(BLESimpleServer* pServer);

    private:
        BLEComServer &m_server;
    };

    class AdvertisedDeviceCallbacks: public BLESimpleCharacteristicCallbacks
    {
    public:
        AdvertisedDeviceCallbacks(BLEComServer& server);
        void onRead(BLESimpleCharacteristic* pCharacteristic);
        void onWrite(BLESimpleCharacteristic *pCharacteristic);

    private:
        BLEComServer &m_server;
    };

    void init_ble_server();
    void restart_advertising();
    bool is_running();

    bool get_connect_info(std::string &ssid, std::string &passphrase);

    void stop();

    void write_back_notify();

    unsigned long get_structure_millis();

private:
    BLESimpleServer* m_pServer;

    std::string m_ssid;
    std::string m_passphrase;
    std::string m_oid;

    unsigned long _structure_period;
    
private:
    friend class AdvertisedDeviceCallbacks;
};

#ifdef __cplusplus
}
#endif


#endif // CUSTOM_BLE_SERVER_COM_INCLUDED