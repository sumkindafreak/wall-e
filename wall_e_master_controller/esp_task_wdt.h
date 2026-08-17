// ============================================================
// WALL-E CYD — ESP task watchdog compatibility shim
//
// The CYD currently builds on Arduino-ESP32 2.0.17 / ESP-IDF 4.x, whose
// esp_task_wdt_init() API takes (timeout_seconds, panic). Newer Arduino-ESP32
// 3.x / ESP-IDF 5.x uses esp_task_wdt_config_t instead.
//
// wall_e_master_controller.ino intentionally uses the newer config-struct API.
// This shim preserves that source while translating the call on IDF 4.x.
// ============================================================

#ifndef WALLE_CYD_ESP_TASK_WDT_COMPAT_H
#define WALLE_CYD_ESP_TASK_WDT_COMPAT_H

#include_next <esp_task_wdt.h>
#include <esp_idf_version.h>
#include <stdint.h>

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)

// Mirror the subset of the IDF 5 configuration structure used by WALL-E.
typedef struct {
  uint32_t timeout_ms;
  uint32_t idle_core_mask;
  bool trigger_panic;
} esp_task_wdt_config_t;

static inline esp_err_t walleTaskWdtInitCompat(const esp_task_wdt_config_t* config) {
  if (!config) {
    return ESP_ERR_INVALID_ARG;
  }

  // IDF 4.x accepts whole seconds. Round up so the configured watchdog is
  // never shorter than the requested timeout.
  uint32_t timeoutSeconds = (config->timeout_ms + 999U) / 1000U;
  if (timeoutSeconds == 0U) {
    timeoutSeconds = 1U;
  }

  return esp_task_wdt_init(timeoutSeconds, config->trigger_panic);
}

// Translate only calls made after this header is included. The helper above
// was compiled before the macro exists, so its call reaches the real IDF 4 API.
#define esp_task_wdt_init(config_ptr) walleTaskWdtInitCompat(config_ptr)

#endif  // ESP_IDF_VERSION_MAJOR < 5

#endif  // WALLE_CYD_ESP_TASK_WDT_COMPAT_H
