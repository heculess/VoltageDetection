#include "ble_uuid.h" 
#include <string.h>
#include <assert.h>

static void memrcpy(uint8_t* target, uint8_t* source, uint32_t size) {
	assert(size > 0);
	target += (size - 1); 
	while (size > 0) {
		*target = *source;
		target--;
		source++;
		size--;
	}
} 

BLESimpleUUID::BLESimpleUUID(std::string value) {
	m_valueSet = true;
	if (value.length() == 4) {
		m_uuid.len         = ESP_UUID_LEN_16;
		m_uuid.uuid.uuid16 = 0;
		for(int i=0;i<value.length();){
			uint8_t MSB = value.c_str()[i];
			uint8_t LSB = value.c_str()[i+1];
			
			if(MSB > '9') MSB -= 7;
			if(LSB > '9') LSB -= 7;
			m_uuid.uuid.uuid16 += (((MSB&0x0F) <<4) | (LSB & 0x0F))<<(2-i)*4;
			i+=2;	
		}
	}
	else if (value.length() == 8) {
		m_uuid.len         = ESP_UUID_LEN_32;
		m_uuid.uuid.uuid32 = 0;
		for(int i=0;i<value.length();){
			uint8_t MSB = value.c_str()[i];
			uint8_t LSB = value.c_str()[i+1];
			
			if(MSB > '9') MSB -= 7; 
			if(LSB > '9') LSB -= 7;
			m_uuid.uuid.uuid32 += (((MSB&0x0F) <<4) | (LSB & 0x0F))<<(6-i)*4;
			i+=2;
		}		
	}
	else if (value.length() == 16) {  // how we can have 16 byte length string reprezenting 128 bit uuid??? needs to be investigated (lack of time)
		m_uuid.len = ESP_UUID_LEN_128;
		memrcpy(m_uuid.uuid.uuid128, (uint8_t*)value.data(), 16);
	}
	else if (value.length() == 36) {
		m_uuid.len = ESP_UUID_LEN_128;
		int n = 0;
		for(int i=0;i<value.length();){
			if(value.c_str()[i] == '-')
				i++;
			uint8_t MSB = value.c_str()[i];
			uint8_t LSB = value.c_str()[i+1];
			
			if(MSB > '9') MSB -= 7; 
			if(LSB > '9') LSB -= 7;
			m_uuid.uuid.uuid128[15-n++] = ((MSB&0x0F) <<4) | (LSB & 0x0F);
			i+=2;	
		}
	}
	else {
		m_valueSet = false;
	}
} 

BLESimpleUUID::BLESimpleUUID(uint8_t* pData, size_t size, bool msbFirst) {
	if (size != 16) 
		return;

	m_uuid.len = ESP_UUID_LEN_128;
	if (msbFirst) {
		memrcpy(m_uuid.uuid.uuid128, pData, 16);
	} else {
		memcpy(m_uuid.uuid.uuid128, pData, 16);
	}
	m_valueSet = true;
} 


BLESimpleUUID::BLESimpleUUID(uint16_t uuid) {
	m_uuid.len         = ESP_UUID_LEN_16;
	m_uuid.uuid.uuid16 = uuid;
	m_valueSet         = true;
} 


BLESimpleUUID::BLESimpleUUID(uint32_t uuid) {
	m_uuid.len         = ESP_UUID_LEN_32;
	m_uuid.uuid.uuid32 = uuid;
	m_valueSet         = true;
} 

BLESimpleUUID::BLESimpleUUID(esp_bt_uuid_t uuid) {
	m_uuid     = uuid;
	m_valueSet = true;
} 

BLESimpleUUID::BLESimpleUUID(esp_gatt_id_t gattId) : BLESimpleUUID(gattId.uuid) {
} 


BLESimpleUUID::BLESimpleUUID() {
	m_valueSet = false;
} 

uint8_t BLESimpleUUID::bitSize() {
	if (!m_valueSet) return 0;
	switch (m_uuid.len) {
		case ESP_UUID_LEN_16:
			return 16;
		case ESP_UUID_LEN_32:
			return 32;
		case ESP_UUID_LEN_128:
			return 128;
		default:
			return 0;
	} 
} 

bool BLESimpleUUID::equals(BLESimpleUUID uuid) {

	if (!m_valueSet || !uuid.m_valueSet) return false;

	if (uuid.m_uuid.len != m_uuid.len) {
		return uuid.toString() == toString();
	}

	if (uuid.m_uuid.len == ESP_UUID_LEN_16) {
		return uuid.m_uuid.uuid.uuid16 == m_uuid.uuid.uuid16;
	}

	if (uuid.m_uuid.len == ESP_UUID_LEN_32) {
		return uuid.m_uuid.uuid.uuid32 == m_uuid.uuid.uuid32;
	}

	return memcmp(uuid.m_uuid.uuid.uuid128, m_uuid.uuid.uuid128, 16) == 0;
} 

BLESimpleUUID BLESimpleUUID::fromString(std::string _uuid) {
	uint8_t start = 0;
	if (strstr(_uuid.c_str(), "0x") != nullptr) { // If the string starts with 0x, skip those characters.
		start = 2;
	}
	uint8_t len = _uuid.length() - start; // Calculate the length of the string we are going to use.

	if(len == 4) {
		uint16_t x = strtoul(_uuid.substr(start, len).c_str(), NULL, 16);
		return BLESimpleUUID(x);
	} else if (len == 8) {
		uint32_t x = strtoul(_uuid.substr(start, len).c_str(), NULL, 16);
		return BLESimpleUUID(x);
	} else if (len == 36) {
		return BLESimpleUUID(_uuid);
	}
	return BLESimpleUUID();
} 

esp_bt_uuid_t* BLESimpleUUID::getNative() {
	if (m_valueSet == false) 
		return nullptr;

	return &m_uuid;
} 

BLESimpleUUID BLESimpleUUID::to128() {
	
	if (!m_valueSet || m_uuid.len == ESP_UUID_LEN_128) {
		return *this;
	}

	if (m_uuid.len == ESP_UUID_LEN_16) {
		uint16_t temp = m_uuid.uuid.uuid16;
		m_uuid.uuid.uuid128[15] = 0;
		m_uuid.uuid.uuid128[14] = 0;
		m_uuid.uuid.uuid128[13] = (temp >> 8) & 0xff;
		m_uuid.uuid.uuid128[12] = temp & 0xff;

	}
	else if (m_uuid.len == ESP_UUID_LEN_32) {
		uint32_t temp = m_uuid.uuid.uuid32;
		m_uuid.uuid.uuid128[15] = (temp >> 24) & 0xff;
		m_uuid.uuid.uuid128[14] = (temp >> 16) & 0xff;
		m_uuid.uuid.uuid128[13] = (temp >> 8) & 0xff;
		m_uuid.uuid.uuid128[12] = temp & 0xff;
	}

	// Set the fixed parts of the UUID.
	m_uuid.uuid.uuid128[11] = 0x00;
	m_uuid.uuid.uuid128[10] = 0x00;

	m_uuid.uuid.uuid128[9]  = 0x10;
	m_uuid.uuid.uuid128[8]  = 0x00;

	m_uuid.uuid.uuid128[7]  = 0x80;
	m_uuid.uuid.uuid128[6]  = 0x00;

	m_uuid.uuid.uuid128[5]  = 0x00;
	m_uuid.uuid.uuid128[4]  = 0x80;
	m_uuid.uuid.uuid128[3]  = 0x5f;
	m_uuid.uuid.uuid128[2]  = 0x9b;
	m_uuid.uuid.uuid128[1]  = 0x34;
	m_uuid.uuid.uuid128[0]  = 0xfb;

	m_uuid.len = ESP_UUID_LEN_128;
	return *this;
} 

std::string BLESimpleUUID::toString() {
	if (!m_valueSet) return "<NULL>";   // If we have no value, nothing to format.

	if (m_uuid.len == ESP_UUID_LEN_16) {  // If the UUID is 16bit, pad correctly.
		char hex[9];
		snprintf(hex, sizeof(hex), "%08x", m_uuid.uuid.uuid16);
		return std::string(hex) + "-0000-1000-8000-00805f9b34fb";
	} // End 16bit UUID

	if (m_uuid.len == ESP_UUID_LEN_32) {  // If the UUID is 32bit, pad correctly.
		char hex[9];
		snprintf(hex, sizeof(hex), "%08x", m_uuid.uuid.uuid32);
		return std::string(hex) + "-0000-1000-8000-00805f9b34fb";
	} // End 32bit UUID

	auto size = 37; // 32 for UUID data, 4 for '-' delimiters and one for a terminator == 37 chars
	char *hex = (char *)malloc(size);
	snprintf(hex, size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			m_uuid.uuid.uuid128[15], m_uuid.uuid.uuid128[14],
			m_uuid.uuid.uuid128[13], m_uuid.uuid.uuid128[12],
			m_uuid.uuid.uuid128[11], m_uuid.uuid.uuid128[10],
			m_uuid.uuid.uuid128[9], m_uuid.uuid.uuid128[8],
			m_uuid.uuid.uuid128[7], m_uuid.uuid.uuid128[6],
			m_uuid.uuid.uuid128[5], m_uuid.uuid.uuid128[4],
			m_uuid.uuid.uuid128[3], m_uuid.uuid.uuid128[2],
			m_uuid.uuid.uuid128[1], m_uuid.uuid.uuid128[0]);
	std::string res(hex);
	free(hex);
	return res;
} 