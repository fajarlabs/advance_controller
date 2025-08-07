#ifndef NVSHELPER_H
#define NVSHELPER_H

#include "nvs_flash.h"
#include "nvs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void init_nvs();
esp_err_t save_nvs_str(const char *namespace, const char *key, const char *value);
esp_err_t save_nvs_i32(const char *namespace, const char *key, int32_t value);
esp_err_t load_nvs_i32(const char *namespace, const char *key, int32_t *out_value);
esp_err_t load_nvs_str(const char *namespace, const char *key, char *out_value, size_t out_value_size);
void load_nvs_data();

#ifdef __cplusplus
}
#endif

#endif // NVSHELPER_H