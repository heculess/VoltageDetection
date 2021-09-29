#ifndef BLE_SIMPLE_VALUE_INCLUDED
#define BLE_SIMPLE_VALUE_INCLUDED

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleValue {
public:
	BLESimpleValue();
	void		addPart(std::string part);
	void		addPart(uint8_t* pData, size_t length);
	void		cancel();
	void		commit();
	size_t	    getLength();
	uint16_t	getReadOffset();
	std::string getValue();
	void        setReadOffset(uint16_t readOffset);
	void        setValue(std::string value);
	void        setValue(uint8_t* pData, size_t length);

private:
	std::string m_accumulation;
	uint16_t    m_readOffset;
	std::string m_value;

};

#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_VALUE_INCLUDED