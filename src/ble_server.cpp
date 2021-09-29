#include "ble_server.h"
#include "ble_descriptor.h"
#include "hal_misc.h"
#include <string>
#include <algorithm>
#include "json_util.h"
#include "esp_log.h"
#include "device_core.h"

static const char *TAG = "BLE";

#define SERVICE_UUID "c46b1535-b3fc-4848-884a-3ca2d70cb66f"
#define CHARACTERISTIC_UUID "e561ce8b-978e-46dc-9989-fd102124f395"
#define CHARACTERISTIC_UUID_CID "41a2f0dc-38a2-474b-9908-dbf96f57a719"

#define JSON_WIFI_SSID "wifi"
#define JSON_WIFI_PASSPHRASE "pwd"
#define JSON_OID "oid"

BLEComServer::ConnectEvtCallbacks::ConnectEvtCallbacks(BLEComServer &server) : m_server(server)
{
}

void BLEComServer::ConnectEvtCallbacks::onConnect(BLESimpleServer *pServer)
{
    BLESimpleServerDevice::startAdvertising();
}

void BLEComServer::ConnectEvtCallbacks::onDisconnect(BLESimpleServer *pServer)
{
}

BLEComServer::AdvertisedDeviceCallbacks::AdvertisedDeviceCallbacks(BLEComServer &server) : m_server(server)
{
}

void BLEComServer::AdvertisedDeviceCallbacks::onRead(BLESimpleCharacteristic *pCharacteristic)
{
    std::string value = pCharacteristic->getValue();
    ESP_LOGI(TAG, "BLEComServer : on read value: %s", value.c_str());
}

void BLEComServer::AdvertisedDeviceCallbacks::onWrite(BLESimpleCharacteristic *pCharacteristic)
{
    std::string value = pCharacteristic->getValue();
    ESP_LOGI(TAG, "BLEComServer : on write value: %s", value.c_str());

    JsonDeserializer json(value.c_str());

    if (!json.has_key(JSON_WIFI_SSID) || !json.has_key(JSON_WIFI_PASSPHRASE) || !json.has_key(JSON_OID))
    {
        ESP_LOGE(TAG, "Error json object, %s", value.c_str());
        return;
    }

    m_server.m_ssid = json.get_value_string(JSON_WIFI_SSID);
    m_server.m_passphrase = json.get_value_string(JSON_WIFI_PASSPHRASE);
    m_server.m_oid = json.get_value_string(JSON_OID);
}

BLEComServer::BLEComServer() : m_pServer(NULL),
                               _structure_period(millis())
{
    init_ble_server();
}

BLEComServer::~BLEComServer()
{
    stop();
}

void BLEComServer::init_ble_server()
{
    std::string device_id = DeviceCore::device_id();
    BLESimpleServerDevice::init(std::string("Pangpi_") + device_id);
    m_pServer = BLESimpleServerDevice::createServer();
    if (!m_pServer)
    {
        ESP_LOGE(TAG, "Create BLE server failed---------------->");
        return;
    }

    m_pServer->setCallbacks(new ConnectEvtCallbacks(*this));
    BLESimpleService *pService = m_pServer->createService(SERVICE_UUID);

    if (!pService)
    {
        ESP_LOGE(TAG, "Create connect service failed---------------->");
        return;
    }

    BLESimpleCharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_CID,
        BLESimpleCharacteristic::PROPERTY_READ);
    pCharacteristic->setValue(device_id);

    BLESimpleCharacteristic *pCharacter_get_wifi_info = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLESimpleCharacteristic::PROPERTY_READ |
            BLESimpleCharacteristic::PROPERTY_WRITE |
            BLESimpleCharacteristic::PROPERTY_NOTIFY |
            BLESimpleCharacteristic::PROPERTY_INDICATE);
    pCharacter_get_wifi_info->setCallbacks(new AdvertisedDeviceCallbacks(*this));
    pCharacter_get_wifi_info->addDescriptor(new BLES2902());
    pCharacter_get_wifi_info->setValue("Hello Pangpi!");

    pService->start();

    BLESimpleAdvertising *pAdvertising = BLESimpleServerDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    //pAdvertising->setMinPreferred(0x12);
    BLESimpleServerDevice::startAdvertising();
}

bool BLEComServer::get_connect_info(std::string &ssid, std::string &passphrase)
{
    printf("B");

    if (m_ssid.empty())
    {
        return false;
    }

    printf("get_connect_info: %s, %s\r\n", m_ssid.c_str(), m_passphrase.c_str());

    write_back_notify();
    ssid = m_ssid;
    passphrase = m_passphrase;
    return true;
}

void BLEComServer::restart_advertising()
{
    if (!m_pServer)
        return;
    BLESimpleServerDevice::startAdvertising();
}

void BLEComServer::stop()
{
    if (!m_pServer)
    {
        return;
    }

    BLESimpleService *pService = m_pServer->getServiceByUUID(SERVICE_UUID);
    if (pService)
    {
        pService->stop();
    }

    BLESimpleServerDevice::stopAdvertising();
}

bool BLEComServer::is_running()
{
    if (!m_pServer)
    {
        return false;
    }

    if (m_pServer->getServiceByUUID(SERVICE_UUID))
    {
        return true;
    }
    return false;
}

void BLEComServer::write_back_notify()
{
    if (!m_pServer)
        return;

    BLESimpleService *pService = m_pServer->getServiceByUUID(SERVICE_UUID);
    if (!pService)
        return;

    BLESimpleCharacteristic *pCharacteristic = pService->getCharacteristic(CHARACTERISTIC_UUID);
    if (!pCharacteristic)
        return;

    std::string notify_value = m_oid + ":200";
    pCharacteristic->setValue(notify_value);
    pCharacteristic->notify();

    ESP_LOGI(TAG, "Characteristic notify, data : %s", notify_value.c_str());
}

unsigned long BLEComServer::get_structure_millis()
{
    return millis() - _structure_period;
}
