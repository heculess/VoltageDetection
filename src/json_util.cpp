#include "json_util.h"
#include "esp_heap_caps.h"

static cJSON_Hooks _allocate_hooks;

JsonPsramInit::JsonPsramInit()
{
    _allocate_hooks.malloc_fn = allocate;
    _allocate_hooks.free_fn = deallocate;
    cJSON_InitHooks(&_allocate_hooks);
}

void *JsonPsramInit::allocate(size_t size)
{
#ifdef CONFIG_ESP32_SPIRAM_SUPPORT
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return heap_caps_malloc(size, MALLOC_CAP_8BIT);
#endif
}

void JsonPsramInit::deallocate(void *ptr)
{
    heap_caps_free(ptr);
}

void *JsonPsramInit::reallocate(void *ptr, size_t new_size)
{
#ifdef CONFIG_ESP32_SPIRAM_SUPPORT
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_8BIT);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//  JsonDeserializer
//
///////////////////////////////////////////////////////////////////////////////////////////

JsonDeserializer::JsonDeserializer(const char *content) : _root(cJSON_Parse(content))
{
    _dont_free = false;
    if (!_root)
        printf("root json is NULL\r\n");
}

JsonDeserializer::JsonDeserializer(cJSON *content) : _root(content)
{
    _dont_free = true;
    if (!_root)
        printf("root json is NULL\r\n");
}

JsonDeserializer::~JsonDeserializer()
{
    if (_root && !_dont_free)
        cJSON_Delete(_root);
}

bool JsonDeserializer::is_empty()
{
    if (!_root)
        return true;
    if (cJSON_GetArraySize(_root) == 0)
        return true;

    return false;
}

bool JsonDeserializer::has_key(const char *key)
{
    if (_root)
        return cJSON_HasObjectItem(_root, key);
    return false;
}

cJSON *JsonDeserializer::get_value(const char *key)
{
    if (_root)
        return cJSON_GetObjectItem(_root, key);
    return NULL;
}

std::string JsonDeserializer::get_value_string(const char *key, const char *default_value)
{
    cJSON *value = get_value(key);
    if (value == NULL)
        return std::string(default_value);

    if (cJSON_IsString(value) || cJSON_IsRaw(value))
        return std::string(value->valuestring);
  
    char * uf_value = cJSON_PrintUnformatted(value);
    std::string str_value(uf_value);
    cJSON_free(uf_value);
    return str_value;
}

int JsonDeserializer::get_value_int(const char *key, int default_value)
{
    cJSON *value = get_value(key);
    if (cJSON_IsNumber(value))
        return value == NULL ? default_value : value->valueint;
    return default_value;
}

bool JsonDeserializer::get_value_bool(const char *key, bool default_value)
{
    cJSON *value = get_value(key);
    if (value == NULL)
        return default_value;
    if (cJSON_IsTrue(value))
        return true;
    else if (cJSON_IsFalse(value))
        return false;
    return default_value;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//  JsonSerializer
//
///////////////////////////////////////////////////////////////////////////////////////////

JsonSerializer::JsonSerializer() : _root(cJSON_CreateObject())
{
}

JsonSerializer::~JsonSerializer()
{
    if (_root)
    {
        cJSON_Delete(_root);
    }
}

bool JsonSerializer::add_value(const char *key, const char *value)
{
    if (!_root)
        return false;

    cJSON_AddStringToObject(_root, key, value);
    return true;
}

bool JsonSerializer::add_value(const char *key, int value)
{
    if (!_root)
        return false;

    cJSON_AddNumberToObject(_root, key, value);
    return true;
}

std::string JsonSerializer::serialize()
{
    char *output = cJSON_PrintUnformatted(_root);
    std::string value(output);
    cJSON_free(output);
    return value;
}