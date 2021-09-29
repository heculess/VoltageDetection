
extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include <esp_log.h>
}


#include "hal_misc.h"
#include "SPIFFS.h"

static const char *TAG = "SPIFFSFS";

SPIFFSFS::DirEnumerator::DirEnumerator(const char* path):
_dir_path(opendir(path))
{
    if (!_dir_path)
    {
        ESP_LOGE(TAG, "Failed to open directory\r\n");
    }
}

SPIFFSFS::DirEnumerator::~DirEnumerator()
{
    if(_dir_path)
        closedir(_dir_path);
}

struct dirent* SPIFFSFS::DirEnumerator::read_dir()
{
    return readdir(_dir_path);
}


////////////////////////////////////////////////////////////////////////////////////////

SPIFFSFS::SPIFFSFS() :  
partitionLabel_(NULL)
{

}

SPIFFSFS::~SPIFFSFS()
{
    if (partitionLabel_){
        free(partitionLabel_);
        partitionLabel_ = NULL;
    }
}

bool SPIFFSFS::begin(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles, const char * partitionLabel)
{
    if (partitionLabel_){
        free(partitionLabel_);
        partitionLabel_ = NULL;
    }

    if (partitionLabel){
        partitionLabel_ = strdup(partitionLabel);
    }

    if(esp_spiffs_mounted(partitionLabel_)){
        ESP_LOGW(TAG, "SPIFFS Already Mounted!");
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
      .base_path = basePath,
      .partition_label = partitionLabel_,
      .max_files = maxOpenFiles,
      .format_if_mount_failed = false
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if(err == ESP_FAIL && formatOnFail){
        if(format()){
            err = esp_vfs_spiffs_register(&conf);
        }
    }
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Mounting SPIFFS failed! Error: %d", err);
        return false;
    }
    _mountpoint = basePath;
    return true;
}



void SPIFFSFS::end()
{
    if(esp_spiffs_mounted(partitionLabel_)){
        esp_err_t err = esp_vfs_spiffs_unregister(partitionLabel_);
        if(err){
            ESP_LOGE(TAG, "Unmounting SPIFFS failed! Error: %d", err);
            return;
        }
        _mountpoint.clear();
    }
}

bool SPIFFSFS::format()
{
    esp_err_t err = ESP_OK;

    {
        CoreWDTLocker locker(0);
        err = esp_spiffs_format(partitionLabel_);
    }

    if(err){
        ESP_LOGE(TAG, "Formatting SPIFFS failed! Error: %d", err);
        return false;
    }
    return true;
}

size_t SPIFFSFS::totalBytes()
{
    size_t total,used;
    if(esp_spiffs_info(partitionLabel_, &total, &used)){
        return 0;
    }
    return total;
}

size_t SPIFFSFS::usedBytes()
{
    size_t total,used;
    if(esp_spiffs_info(partitionLabel_, &total, &used)){
        return 0;
    }
    return used;
}

std::string SPIFFSFS::get_full_path(const char * path)
{
    std::string full_path(_mountpoint);
    full_path.append(path);
    return full_path;
}

FILE* SPIFFSFS::open(const char* path, const char* mode)
{
    return fopen(get_full_path(path).c_str(), mode);
}

bool SPIFFSFS::exists(const char* path)
{
    struct stat st;
    if (stat(get_full_path(path).c_str(), &st) == 0) {
        return true;
    }
    return false;
}

void SPIFFSFS::remove(const char* path)
{
    unlink(get_full_path(path).c_str());
}

size_t SPIFFSFS::file_size(const char* path)
{
    struct stat st;
    if (stat(get_full_path(path).c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

SPIFFSFS SPIFFS;


SPIFFSFILE::SPIFFSFILE(SemaphoreHandle_t mutex_handle):
_h_file(NULL),
_mutex_handle(mutex_handle)
{
    if(_mutex_handle){
        xSemaphoreTake(_mutex_handle, portMAX_DELAY);
        ESP_LOGD(TAG, "xSemaphoreTake");
    }
}

SPIFFSFILE::~SPIFFSFILE()
{
    if(_h_file){
        fclose(_h_file);
        _h_file = NULL;
    }

    if(_mutex_handle){
        ESP_LOGD(TAG, "xSemaphoreGive");
        xSemaphoreGive(_mutex_handle);
    }
}


bool SPIFFSFILE::open(const char* path, const char* mode)
{
    _file_path = path;
    _h_file = SPIFFS.open(path,mode);
    if(_h_file)
        return true;
    return false;
}

bool SPIFFSFILE::exists(const char* path)
{
    return SPIFFS.exists(path);
}

void SPIFFSFILE::remove(const char* path)
{
    return SPIFFS.remove(path);
}

bool SPIFFSFILE::exists(const char* path, SemaphoreHandle_t mutex_handle)
{
    SPIFFSFILE file(mutex_handle);
    return SPIFFS.exists(path);
}

void SPIFFSFILE::remove(const char* path, SemaphoreHandle_t mutex_handle)
{
    SPIFFSFILE file(mutex_handle);
    return SPIFFS.remove(path);
}

size_t SPIFFSFILE::file_size()
{
    return SPIFFS.file_size(_file_path.c_str());
}

size_t SPIFFSFILE::read(void * buffer, size_t size)
{
    return fread(buffer, 1, size, _h_file);
}

size_t SPIFFSFILE::write(const void * buffer , size_t size)
{
    return fwrite(buffer, 1, size, _h_file);
}

size_t SPIFFSFILE::write(const char * buffer)
{
    return fwrite(buffer, 1, strlen(buffer), _h_file);
}

