#include "Update.h"
#include "esp.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "driver/gpio.h"
#include <string.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "UpdateClass";

static const char * _err2str(uint8_t _error){
    if(_error == UPDATE_ERROR_OK){
        return ("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        return ("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        return ("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        return ("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        return ("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        return ("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        return ("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        return ("MD5 Check Failed");
    } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
        return ("Wrong Magic Byte");
    } else if(_error == UPDATE_ERROR_ACTIVATE){
        return ("Could Not Activate The Firmware");
    } else if(_error == UPDATE_ERROR_NO_PARTITION){
        return ("Partition Could Not be Found");
    } else if(_error == UPDATE_ERROR_BAD_ARGUMENT){
        return ("Bad Argument");
    } else if(_error == UPDATE_ERROR_ABORT){
        return ("Aborted");
    }
    return ("UNKNOWN");
}

static bool _partitionIsBootable(const esp_partition_t* partition){
    uint8_t buf[ENCRYPTED_BLOCK_SIZE];
    if(!partition){
        return false;
    }
    if(!ESP.partitionRead(partition, 0, (uint32_t*)buf, ENCRYPTED_BLOCK_SIZE)) {
        return false;
    }

    if(buf[0] != ESP_IMAGE_HEADER_MAGIC) {
        return false;
    }
    return true;
}

bool UpdateClass::_enablePartition(const esp_partition_t* partition){
    if(!partition){
        return false;
    }
    return ESP.partitionWrite(partition, 0, (uint32_t*) _skipBuffer, ENCRYPTED_BLOCK_SIZE);
}

UpdateClass::UpdateClass()
: _error(0)
, _buffer(0)
, _bufferLen(0)
, _size(0)
, _progress_callback(NULL)
, _progress(0)
, _command(U_FLASH)
, _partition(NULL)
{
}

UpdateClass& UpdateClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void UpdateClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _progress = 0;
    _size = 0;
    _command = U_FLASH;

    if(_ledPin != -1) {
      gpio_set_level((gpio_num_t)_ledPin, !_ledOn);
    }
}

bool UpdateClass::canRollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition);
}

bool UpdateClass::rollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition) && !esp_ota_set_boot_partition(partition);
}

bool UpdateClass::begin(size_t size, int command, int ledPin, uint8_t ledOn, const char *label) {
    if(_size > 0){
        ESP_LOGD(TAG, "already running");
        return false;
    }

    _ledPin = ledPin;
    _ledOn = !!ledOn; // 0(LOW) or 1(HIGH)

    _reset();
    _error = 0;
    _target_md5.clear();
    _md5 = MD5Builder();

    if(size == 0) {
        _error = UPDATE_ERROR_SIZE;
        return false;
    }

    if (command == U_FLASH) {
        _partition = esp_ota_get_next_update_partition(NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
        ESP_LOGD(TAG, "OTA Partition: %s", _partition->label);
    }
    else if (command == U_SPIFFS) {
        _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, label);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
    }
    else {
        _error = UPDATE_ERROR_BAD_ARGUMENT;
        ESP_LOGE(TAG, "bad command %u", command);
        return false;
    }

    if(size == UPDATE_SIZE_UNKNOWN){
        size = _partition->size;
    } else if(size > _partition->size){
        _error = UPDATE_ERROR_SIZE;
        ESP_LOGE(TAG, "too large %u > %u", size, _partition->size);
        return false;
    }

    //initialize
    //_buffer = (uint8_t*)malloc(SPI_FLASH_SEC_SIZE);
    _buffer = (uint8_t*)heap_caps_malloc(SPI_FLASH_SEC_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!_buffer){
        ESP_LOGE(TAG, "malloc failed");
        return false;
    }
    _size = size;
    _command = command;
    return true;
}

void UpdateClass::_abort(uint8_t err){
    _reset();
    _error = err;
}

void UpdateClass::abort(){
    _abort(UPDATE_ERROR_ABORT);
}

bool UpdateClass::_writeBuffer(){
    //first bytes of new firmware
    uint8_t skip = 0;
    if(!_progress && _command == U_FLASH){
        //check magic
        if(_buffer[0] != ESP_IMAGE_HEADER_MAGIC){
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }

        //Stash the first 16 bytes of data and set the offset so they are
        //not written at this point so that partially written firmware
        //will not be bootable
        skip = ENCRYPTED_BLOCK_SIZE;
        //_skipBuffer = (uint8_t*)malloc(skip);
        _skipBuffer = (uint8_t*)heap_caps_malloc(skip, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if(!_skipBuffer){
            ESP_LOGE(TAG, "malloc failed");
        return false;
        }
        memcpy(_skipBuffer, _buffer, skip);
    }
    if (!_progress && _progress_callback) {
        _progress_callback(0, _size);
    }
    if(!ESP.partitionEraseRange(_partition, _progress, SPI_FLASH_SEC_SIZE)){
        _abort(UPDATE_ERROR_ERASE);
        return false;
    }
    if (!ESP.partitionWrite(_partition, _progress + skip, (uint32_t*)_buffer + skip/sizeof(uint32_t), _bufferLen - skip)) {
        _abort(UPDATE_ERROR_WRITE);
        return false;
    }
    //restore magic or md5 will fail
    if(!_progress && _command == U_FLASH){
        _buffer[0] = ESP_IMAGE_HEADER_MAGIC;
    }
    _md5.update(_buffer, _bufferLen);
    _progress += _bufferLen;
    _bufferLen = 0;
    if (_progress_callback) {
        _progress_callback(_progress, _size);
    }
    return true;
}

bool UpdateClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        if(data != ESP_IMAGE_HEADER_MAGIC) {
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_SPIFFS) {
        return true;
    }
    return false;
}

bool UpdateClass::_verifyEnd() {
    if(_command == U_FLASH) {
        if(!_enablePartition(_partition) || !_partitionIsBootable(_partition)) {
            _abort(UPDATE_ERROR_READ);
            return false;
        }

        if(esp_ota_set_boot_partition(_partition)){
            _abort(UPDATE_ERROR_ACTIVATE);
            return false;
        }
        _reset();
        return true;
    } else if(_command == U_SPIFFS) {
        _reset();
        return true;
    }
    return false;
}

bool UpdateClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32)
    {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

bool UpdateClass::end(bool evenIfRemaining){
    if(hasError() || _size == 0){
        return false;
    }

    if(!isFinished() && !evenIfRemaining){
        ESP_LOGE(TAG, "premature end: res:%u, pos:%u/%u\n", getError(), progress(), _size);
        _abort(UPDATE_ERROR_ABORT);
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
            _writeBuffer();
        }
        _size = progress();
    }

    if(_target_md5.length()) {
        if(_target_md5 != _md5.calculate()){
            _abort(UPDATE_ERROR_MD5);
            return false;
        }
    }

    return _verifyEnd();
}

size_t UpdateClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning()){
        return 0;
    }

    if(len > remaining()){
        _abort(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > SPI_FLASH_SEC_SIZE) {
        size_t toBuff = SPI_FLASH_SEC_SIZE - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
    }
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        if(!_writeBuffer()){
            return len - left;
        }
    }
    return len;
}

const char * UpdateClass::errorString(){
    return _err2str(_error);
}

UpdateClass Update;