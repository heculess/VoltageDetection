// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _SPIFFS_H_
#define _SPIFFS_H_

#include <string>
#include "esp_vfs.h"

#define FILE_READ       "r"
#define FILE_WRITE      "w"
#define FILE_APPEND     "a"

class SPIFFSFS 
{

public:
    class DirEnumerator
    {
    public:
        DirEnumerator(const char* path);
        virtual ~DirEnumerator();

        struct dirent* read_dir();

    private:
        DIR* _dir_path;
    };

public:
    SPIFFSFS();
    ~SPIFFSFS();
    bool begin(bool formatOnFail=false, const char * basePath="/spiffs", uint8_t maxOpenFiles=10, const char * partitionLabel=NULL);
    bool format();
    size_t totalBytes();
    size_t usedBytes();
    void end();

    FILE* open(const char* path, const char* mode = FILE_READ);

    bool exists(const char* path);
    void remove(const char* path);

    size_t file_size(const char* path);

    std::string get_full_path(const char * path);

private:
    char * partitionLabel_;
    std::string _mountpoint;  
};

extern SPIFFSFS SPIFFS;


class SPIFFSFILE 
{
public:
    SPIFFSFILE(SemaphoreHandle_t mutex_handle);
    virtual ~SPIFFSFILE();

    bool open(const char* path, const char* mode = FILE_READ);

    bool exists(const char* path);
    void remove(const char* path);

    static bool exists(const char* path, SemaphoreHandle_t mutex_handle);
    static void remove(const char* path, SemaphoreHandle_t mutex_handle);

    size_t file_size();

    size_t read(void * buffer, size_t size);
    size_t write(const void * buffer , size_t size);
    size_t write(const char * buffer);

private:
   FILE * _h_file;
   SemaphoreHandle_t _mutex_handle;
   std::string _file_path;
};


#endif
