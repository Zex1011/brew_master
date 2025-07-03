// ConfigManager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <cstdint>
#include <cstddef>
#include "esp_err.h"
#include "freertos/semphr.h"

static constexpr size_t MAX_STEPS = 20;

/* Estrutura gravada na NVS ------------------------------------------------- */
struct BrewConfig {
    uint8_t  step_count;                         // etapas válidas
    int16_t  temperatures[MAX_STEPS];            // °C (ou °C × 10 se preferir)
    uint32_t durations    [MAX_STEPS];           // segundos
};

/* Classe que gerencia RAM + NVS ------------------------------------------- */
class ConfigManager {
public:
    /* ---------- NVS ---------- */
    static esp_err_t init();
    static bool      hasDefaultConfig();
    static esp_err_t clearDefaultConfig();
    static esp_err_t saveToFlash();
    static esp_err_t loadFromFlash();
    static esp_err_t resetToFactory();

    /* ---------- Array em RAM ---------- */
    static size_t   getStepCount();
    static void      op_PushStep(int16_t temp, uint32_t dur);
    static void      op_PopStep();
    static int16_t  getTemperature(size_t idx);
    static uint32_t getDuration   (size_t idx);
    static void     clearSteps();
    static void     printConfig();               // opcional: via UART/log

private:
    static SemaphoreHandle_t mutex_;
    static int16_t  temps_[MAX_STEPS];
    static uint32_t durations_[MAX_STEPS];
    static size_t   stepCount_;
    static const BrewConfig FACTORY_DEFAULT;
};

/* ---- Wrappers chamados pelo statechart (mantidos) ---- */
void op_InitConfig();
void op_LoadConfigFromFlash();
void op_SaveConfigToFlash();
void op_ClearFlashConfig();
void op_ResetToFactory();
void op_PrintConfig();

#endif // CONFIG_MANAGER_H
