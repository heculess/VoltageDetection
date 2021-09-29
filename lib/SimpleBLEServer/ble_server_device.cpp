#include "ble_server_device.h" 
#include <esp_bt_main.h>       // ESP32 BLE
#include <esp_gap_ble_api.h>   // ESP32 BLE
#include <esp_gatts_api.h>     // ESP32 BLE
#include "esp_bt.h"

#include "ble_advertising.h" 


BLESimpleAdvertising* BLESimpleServerDevice::m_bleAdvertising = nullptr;
BLESimpleServer* BLESimpleServerDevice::m_pServer = nullptr;
uint16_t BLESimpleServerDevice::m_appId = 0;
bool BLESimpleServerDevice::m_initialized = false;


void BLESimpleServerDevice::init(std::string deviceName) {

	if(!m_initialized){
		m_initialized = true; 

		esp_err_t errRc = ESP_OK;

		if (!btStart()) {
			errRc = ESP_FAIL;
			return;
		}

		esp_bluedroid_status_t bt_state = esp_bluedroid_get_status();
		if (bt_state == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
			errRc = esp_bluedroid_init();
			if (errRc != ESP_OK) 
				return;
		}

		if (bt_state != ESP_BLUEDROID_STATUS_ENABLED) {
			errRc = esp_bluedroid_enable();
			if (errRc != ESP_OK) 
				return;
		}

		errRc = esp_ble_gatts_register_callback(BLESimpleServerDevice::gattServerEventHandler);
		if (errRc != ESP_OK) 
			return;

		errRc = ::esp_ble_gap_set_device_name(deviceName.c_str());
		if (errRc != ESP_OK) 
			return;
	}
	vTaskDelay(200 / portTICK_PERIOD_MS);
} 

bool BLESimpleServerDevice::btStart(){
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        esp_bt_controller_init(&cfg);
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){}
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if (esp_bt_controller_enable(ESP_BT_MODE_BTDM)) {
            return false;
        }
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    return false;
}

BLESimpleServer* BLESimpleServerDevice::createServer() {
	m_pServer = new BLESimpleServer();
	m_pServer->createApp(m_appId++);
	return m_pServer;
} 

BLESimpleAdvertising* BLESimpleServerDevice::getAdvertising() {

	if(m_bleAdvertising == nullptr) 
		m_bleAdvertising = new BLESimpleAdvertising();

	return m_bleAdvertising; 
}

void BLESimpleServerDevice::startAdvertising() {
	getAdvertising()->start();
} 

void BLESimpleServerDevice::stopAdvertising() {
    getAdvertising()->stop();
} 

void BLESimpleServerDevice::gattServerEventHandler(esp_gatts_cb_event_t event,
   esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	
	if (BLESimpleServerDevice::m_pServer != nullptr) 
		BLESimpleServerDevice::m_pServer->handleGATTServerEvent(event, gatts_if, param);
} 