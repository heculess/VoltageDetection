#ifndef _JSON_UTIL_H
#define _JSON_UTIL_H

#include "cJSON.h"
#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

  class JsonPsramInit
  {
  public:
    JsonPsramInit();

  private:
    static void *allocate(size_t size);
    static void deallocate(void *ptr);
    static void *reallocate(void *ptr, size_t new_size);
  };
  class JsonDeserializer : public JsonPsramInit
  {
  public:
    JsonDeserializer(const char *content);
    JsonDeserializer(cJSON *content);
    virtual ~JsonDeserializer();

    bool is_empty();

    bool has_key(const char *key);
    cJSON *get_value(const char *key);
    std::string get_value_string(const char *key, const char* default_value="");
    int get_value_int(const char *key, int default_value);
    bool get_value_bool(const char *key, bool default_value);

  private:
    bool _dont_free;
    cJSON *_root;
  };

  class JsonSerializer : public JsonPsramInit
  {
  public:
    JsonSerializer();
    virtual ~JsonSerializer();

    bool add_value(const char *key, const char *value);
    bool add_value(const char *key, int value);

    std::string serialize();

  private:
    cJSON *_root;
  };

#ifdef __cplusplus
}
#endif

#endif