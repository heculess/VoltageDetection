#include <string.h>
#include "ble_advertising.h" 

BLESimpleAdvertising::BLESimpleAdvertising()
: m_scanRespData{}
{
	m_advData.set_scan_rsp        = false;
	m_advData.include_name        = true;
	m_advData.include_txpower     = true;
	m_advData.min_interval        = 0x20;
	m_advData.max_interval        = 0x40;
	m_advData.appearance          = 0x00;
	m_advData.manufacturer_len    = 0;
	m_advData.p_manufacturer_data = nullptr;
	m_advData.service_data_len    = 0;
	m_advData.p_service_data      = nullptr;
	m_advData.service_uuid_len    = 0;
	m_advData.p_service_uuid      = nullptr;
	m_advData.flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

	m_advParams.adv_int_min       = 0x20;
	m_advParams.adv_int_max       = 0x40;
	m_advParams.adv_type          = ADV_TYPE_IND;
	m_advParams.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
	m_advParams.channel_map       = ADV_CHNL_ALL;
	m_advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
	m_advParams.peer_addr_type    = BLE_ADDR_TYPE_PUBLIC;
} 

void BLESimpleAdvertising::addServiceUUID(BLESimpleUUID serviceUUID) {
	m_serviceUUIDs.push_back(serviceUUID);
} 

void BLESimpleAdvertising::addServiceUUID(const char* serviceUUID) {
	addServiceUUID(BLESimpleUUID(serviceUUID));
} 

void BLESimpleAdvertising::setMinInterval(uint16_t mininterval) {
	m_advParams.adv_int_min = mininterval;
} 

void BLESimpleAdvertising::setMaxInterval(uint16_t maxinterval) {
	m_advParams.adv_int_max = maxinterval;
} 

void BLESimpleAdvertising::setMinPreferred(uint16_t mininterval) {
	m_advData.min_interval = mininterval;
} 

void BLESimpleAdvertising::setMaxPreferred(uint16_t maxinterval) {
	m_advData.max_interval = maxinterval;
} 

void BLESimpleAdvertising::setScanResponse(bool set) {
	m_scanResp = set;
}

void BLESimpleAdvertising::start() {
	
	int numServices = m_serviceUUIDs.size();
	if (numServices > 0) {
		m_advData.service_uuid_len = 16 * numServices;
		m_advData.p_service_uuid = new uint8_t[m_advData.service_uuid_len];
		uint8_t* p = m_advData.p_service_uuid;
		for (int i = 0; i < numServices; i++) {
			BLESimpleUUID serviceUUID128 = m_serviceUUIDs[i].to128();
			memcpy(p, serviceUUID128.getNative()->uuid.uuid128, 16);
			p += 16;
		}
	} else {
		m_advData.service_uuid_len = 0;
	}

    m_advData.set_scan_rsp = false;
    m_advData.include_name = !m_scanResp;
    m_advData.include_txpower = !m_scanResp;
    ::esp_ble_gap_config_adv_data(&m_advData);

    memcpy(&m_scanRespData, &m_advData, sizeof(esp_ble_adv_data_t)); // Copy the content of m_advData.
    m_scanRespData.set_scan_rsp = true; // Define this struct as scan response data
    m_scanRespData.include_name = true; // Caution: This may lead to a crash if the device name has more than 29 characters
    m_scanRespData.include_txpower = true;
    m_scanRespData.appearance = 0; // If defined the 'Appearance' attribute is already included in the advertising data
    m_scanRespData.flag = 0; // 'Flags' attribute should no be included in the scan response

    if (::esp_ble_gap_config_adv_data(&m_scanRespData) != ESP_OK){
		return;
	}
        

	if (m_advData.service_uuid_len > 0) {
		delete[] m_advData.p_service_uuid;
		m_advData.p_service_uuid = nullptr;
	}

	esp_ble_gap_start_advertising(&m_advParams);
} 


void BLESimpleAdvertising::stop() {
	::esp_ble_gap_stop_advertising();
} 

