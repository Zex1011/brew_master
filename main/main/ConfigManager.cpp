// ConfigManager.cpp
#include <Arduino.h>
#include "ConfigManager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstdio>

/* ------------------------------------------------------------------------- */
/*  Estáticos                                                                */
/* ------------------------------------------------------------------------- */
SemaphoreHandle_t ConfigManager::mutex_ = nullptr;
int16_t  ConfigManager::temps_[MAX_STEPS]     = {0};
uint32_t ConfigManager::durations_[MAX_STEPS] = {0};
size_t   ConfigManager::stepCount_            = 0;

/* Default de fábrica ------------------------------------------------------ */
const BrewConfig ConfigManager::FACTORY_DEFAULT = {
    3,
    {20, 65, 20},                 // temperaturas °C
    {3600, 1800, 7200}            // durações  s
};

/* ------------------------------------------------------------------------- */
/*  NVS init/clear/check                                                    */
/* ------------------------------------------------------------------------- */
esp_err_t ConfigManager::init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    mutex_ = xSemaphoreCreateMutex();
    return (mutex_ ? ESP_OK : ESP_ERR_NO_MEM);
}

bool ConfigManager::hasDefaultConfig()
{
    nvs_handle_t h;
    if (nvs_open("brew_cfg", NVS_READONLY, &h) != ESP_OK) return false;
    size_t sz = 0;
    esp_err_t err = nvs_get_blob(h, "default", nullptr, &sz);
    nvs_close(h);
    return (err == ESP_OK && sz == sizeof(BrewConfig));
}

esp_err_t ConfigManager::clearDefaultConfig()
{
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t h;
    esp_err_t err = nvs_open("brew_cfg", NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_erase_key(h, "default");
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
    }
    xSemaphoreGive(mutex_);
    return err;
}

/* ------------------------------------------------------------------------- */
/*  Salvar / Carregar da flash                                              */
/* ------------------------------------------------------------------------- */
esp_err_t ConfigManager::saveToFlash()
{
    BrewConfig cfg {};
    cfg.step_count = static_cast<uint8_t>(stepCount_);
    for (size_t i = 0; i < stepCount_; ++i) {
        cfg.temperatures[i] = temps_[i];
        cfg.durations[i]    = durations_[i];
    }

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t h;
    esp_err_t err = nvs_open("brew_cfg", NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_set_blob(h, "default", &cfg, sizeof(cfg));
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
    }
    xSemaphoreGive(mutex_);
    return err;
}

esp_err_t ConfigManager::loadFromFlash()
{
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t h;
    BrewConfig cfg;
    esp_err_t err = nvs_open("brew_cfg", NVS_READONLY, &h);
    if (err == ESP_OK) {
        size_t sz = sizeof(cfg);
        err = nvs_get_blob(h, "default", &cfg, &sz);
        nvs_close(h);
    }
    xSemaphoreGive(mutex_);
    if (err != ESP_OK) return err;

    clearSteps();
    for (size_t i = 0; i < cfg.step_count; ++i)
        op_PushStep(cfg.temperatures[i], cfg.durations[i]);

    return ESP_OK;
}

esp_err_t ConfigManager::resetToFactory()
{
    clearSteps();
    for (size_t i = 0; i < FACTORY_DEFAULT.step_count; ++i)
        op_PushStep(FACTORY_DEFAULT.temperatures[i], FACTORY_DEFAULT.durations[i]);

    return saveToFlash();
}

/* ------------------------------------------------------------------------- */
/*  Array em RAM                                                            */
/* ------------------------------------------------------------------------- */
size_t ConfigManager::getStepCount() { return stepCount_; }

void ConfigManager::op_PushStep(int16_t temp, uint32_t dur)
{
    if (stepCount_ >= MAX_STEPS);
    temps_[stepCount_]     = temp;
    durations_[stepCount_] = dur;
    ++stepCount_;
}

void ConfigManager::op_PopStep()
{
    if (stepCount_ == 0);
    --stepCount_;
    temps_[stepCount_]     = 0;
    durations_[stepCount_] = 0;
}

int16_t ConfigManager::getTemperature(size_t idx)
{
    return (idx < stepCount_) ? temps_[idx] : INT16_MIN;   // -32768 = inválido
}

uint32_t ConfigManager::getDuration(size_t idx)
{
    return (idx < stepCount_) ? durations_[idx] : 0;
}

void ConfigManager::clearSteps() { stepCount_ = 0; }

void ConfigManager::printConfig()
{
    printf("---- Config atual (%zu etapas) ----\n", stepCount_);
    for (size_t i = 0; i < stepCount_; ++i)
        printf("%2zu) %d °C  %u s\n", i, temps_[i], durations_[i]);
}

/* ------------------------------------------------------------------------- */
/*  Wrappers para o statechart                                              */
/* ------------------------------------------------------------------------- */
void op_InitConfig()               { ConfigManager::init(); }
void op_LoadConfigFromFlash()      { ConfigManager::loadFromFlash(); }
void op_SaveConfigToFlash()        { ConfigManager::saveToFlash(); }
void op_ClearFlashConfig()         { ConfigManager::clearDefaultConfig(); }
void op_ResetToFactory()           { ConfigManager::resetToFactory(); }
void op_PrintConfig()              { ConfigManager::printConfig(); }
