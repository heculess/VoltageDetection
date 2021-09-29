#ifndef ESP32_UPDATER_H
#define ESP32_UPDATER_H

#include "crypto.h"
#include <functional>
#include <string>
#include "esp_partition.h"

#define UPDATE_ERROR_OK                 (0)
#define UPDATE_ERROR_WRITE              (1)
#define UPDATE_ERROR_ERASE              (2)
#define UPDATE_ERROR_READ               (3)
#define UPDATE_ERROR_SPACE              (4)
#define UPDATE_ERROR_SIZE               (5)
#define UPDATE_ERROR_STREAM             (6)
#define UPDATE_ERROR_MD5                (7)
#define UPDATE_ERROR_MAGIC_BYTE         (8)
#define UPDATE_ERROR_ACTIVATE           (9)
#define UPDATE_ERROR_NO_PARTITION       (10)
#define UPDATE_ERROR_BAD_ARGUMENT       (11)
#define UPDATE_ERROR_ABORT              (12)

#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

#define U_FLASH   0
#define U_SPIFFS  100
#define U_AUTH    200

#define ENCRYPTED_BLOCK_SIZE 16

class UpdateClass {
  public:
    typedef std::function<void(size_t, size_t)> THandlerFunction_Progress;

    UpdateClass();

    UpdateClass& onProgress(THandlerFunction_Progress fn);


    bool begin(size_t size=UPDATE_SIZE_UNKNOWN, int command = U_FLASH, int ledPin = -1, uint8_t ledOn = 0, const char *label = NULL);

    size_t write(uint8_t *data, size_t len);

    bool end(bool evenIfRemaining = false);

    void abort();

    const char * errorString();

    bool setMD5(const char * expected_md5);

    uint8_t getError(){ return _error; }
    void clearError(){ _error = UPDATE_ERROR_OK; }
    bool hasError(){ return _error != UPDATE_ERROR_OK; }
    bool isRunning(){ return _size > 0; }
    bool isFinished(){ return _progress == _size; }
    size_t size(){ return _size; }
    size_t progress(){ return _progress; }
    size_t remaining(){ return _size - _progress; }

    bool canRollBack();

    bool rollBack();

  private:
    void _reset();
    void _abort(uint8_t err);
    bool _writeBuffer();
    bool _verifyHeader(uint8_t data);
    bool _verifyEnd();
    bool _enablePartition(const esp_partition_t* partition);


    uint8_t _error;
    uint8_t *_buffer;
    uint8_t *_skipBuffer;
    size_t _bufferLen;
    size_t _size;
    THandlerFunction_Progress _progress_callback;
    uint32_t _progress;
    uint32_t _command;
    const esp_partition_t* _partition;

    std::string _target_md5;
    MD5Builder _md5;

    int _ledPin;
    uint8_t _ledOn;
};

extern UpdateClass Update;

#endif