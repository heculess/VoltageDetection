#ifndef BLE_SIMPLE_ADVERTISING_INCLUDED
#define BLE_SIMPLE_ADVERTISING_INCLUDED

#include <esp_gap_ble_api.h>
#include <vector>
#include "ble_uuid.h" 

#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleAdvertising {
public:
	BLESimpleAdvertising();
	void addServiceUUID(BLESimpleUUID serviceUUID);
	void addServiceUUID(const char* serviceUUID);
	void start();
	void stop();
	void setMaxInterval(uint16_t maxinterval);
	void setMinInterval(uint16_t mininterval);
	void setMinPreferred(uint16_t);
	void setMaxPreferred(uint16_t);
	void setScanResponse(bool);

private:
	esp_ble_adv_data_t   m_advData;
	esp_ble_adv_data_t   m_scanRespData; 
	esp_ble_adv_params_t m_advParams;
	std::vector<BLESimpleUUID> m_serviceUUIDs;
	bool                 m_scanResp = true;

};


#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_ADVERTISING_INCLUDED