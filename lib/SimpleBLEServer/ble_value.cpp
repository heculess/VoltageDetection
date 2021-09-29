#include "ble_value.h" 

BLESimpleValue::BLESimpleValue() {
	m_accumulation = "";
	m_value        = "";
	m_readOffset   = 0;
} 

void BLESimpleValue::addPart(std::string part) {
	m_accumulation += part;
} 

void BLESimpleValue::addPart(uint8_t* pData, size_t length) {
	m_accumulation += std::string((char*) pData, length);
} 

void BLESimpleValue::cancel() {
	m_accumulation = "";
	m_readOffset   = 0;
} 

void BLESimpleValue::commit() {
	if (m_accumulation.length() == 0) return;
	setValue(m_accumulation);
	m_accumulation = "";
	m_readOffset   = 0;
} 

size_t BLESimpleValue::getLength() {
	return m_value.length();
} 

uint16_t BLESimpleValue::getReadOffset() {
	return m_readOffset;
} 

std::string BLESimpleValue::getValue() {
	return m_value;
} 

void BLESimpleValue::setReadOffset(uint16_t readOffset) {
	m_readOffset = readOffset;
} 

void BLESimpleValue::setValue(std::string value) {
	m_value = value;
} 

void BLESimpleValue::setValue(uint8_t* pData, size_t length) {
	m_value = std::string((char*) pData, length);
} 