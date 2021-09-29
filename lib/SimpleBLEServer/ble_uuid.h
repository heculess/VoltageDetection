#ifndef BLE_SIMPLE_UUID_INCLUDED
#define BLE_SIMPLE_UUID_INCLUDED

#include <esp_gatt_defs.h>
#include <string>


#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleUUID {
public:
	BLESimpleUUID(std::string uuid);
	BLESimpleUUID(uint16_t uuid);
	BLESimpleUUID(uint32_t uuid);
	BLESimpleUUID(esp_bt_uuid_t uuid);
	BLESimpleUUID(uint8_t* pData, size_t size, bool msbFirst);
	BLESimpleUUID(esp_gatt_id_t gattId);
	BLESimpleUUID();
	uint8_t        bitSize();   // Get the number of bits in this uuid.
	bool           equals(BLESimpleUUID uuid);
	esp_bt_uuid_t* getNative();
	BLESimpleUUID        to128();
	std::string    toString();
	static BLESimpleUUID fromString(std::string uuid); 

private:
	esp_bt_uuid_t m_uuid;       		// The underlying UUID structure that this class wraps.
	bool          m_valueSet = false;   // Is there a value set for this instance.
}; 

#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_UUID_INCLUDED