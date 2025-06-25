// ConfigManager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <cstdint>
#include <cstddef>
#include "esp_err.h"
#include "freertos/semphr.h"

/**
 * Número máximo de etapas suportadas pelo processo de brassagem.
 */
static constexpr size_t MAX_STEPS = 20;

/**
 * Estrutura que representa uma configuração completa (vetor e persistência).
 */
struct BrewConfig {
    uint8_t   step_count;                     // Número de etapas válidas
    float     temperatures[MAX_STEPS];        // Temperaturas em °C
    uint32_t  durations[MAX_STEPS];           // Durações em segundos
};

/**
 * Gerencia todas as configurações de brassagem: array em RAM + NVS.
 *
 * Combina lógica de push/pop/clear em memória com operações de
 * gravação/carregamento na flash (NVS) do ESP32.
 */
class ConfigManager {
public:
    // ----- Inicialização da NVS -----
    static esp_err_t init();
    static bool      hasDefaultConfig();
    static esp_err_t clearDefaultConfig();

    // ----- Persistência -----
    /**
     * Salva a configuração RAM atual como default na flash.
     */
    static esp_err_t saveToFlash();
    /**
     * Carrega a configuração default da flash para RAM.
     */
    static esp_err_t loadFromFlash();
    /**
     * Restaura o default de fábrica (hard-coded) em RAM e flash.
     */
    static esp_err_t resetToFactory();

    // ----- Gerência do array de etapas em RAM -----
    static size_t    getStepCount();
    static bool      pushStep(float temp, uint32_t duration);
    static bool      popStep();
    static float     getTemperature(size_t idx);
    static uint32_t  getDuration(size_t idx);
    static void      clearSteps();

    /** Imprime via UART todas as etapas (temp + dur). */
    static void printConfig();

private:
    // Mutex para proteger acesso à NVS e ao array
    static SemaphoreHandle_t mutex_;
    // Array em RAM
    static float    temps_[MAX_STEPS];
    static uint32_t durations_[MAX_STEPS];
    static size_t   stepCount_;
    // Default de fábrica
    static const BrewConfig FACTORY_DEFAULT;
};

// Declarações das operações chamadas pelo Statechart
void op_InitConfig();
void op_LoadConfigFromFlash();
void op_SaveConfigToFlash();
void op_ClearFlashConfig();
void op_ResetToFactory();
void op_PrintConfig();

#endif // CONFIG_MANAGER_H

