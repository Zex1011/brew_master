
// ConfigManager.cpp
#include "ConfigManager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "uart.h"         // uart_send(const char*)
#include "Statechart.h"   // fireEvent(id,event)

// Definição dos estáticos
SemaphoreHandle_t ConfigManager::mutex_ = nullptr;
int    ConfigManager::temps_[MAX_STEPS]     = {0};
uint32_t ConfigManager::durations_[MAX_STEPS] = {0};
size_t   ConfigManager::stepCount_            = 0;

// Hard-coded factory default
const BrewConfig ConfigManager::FACTORY_DEFAULT = {
    3,
    {20, 65, 20},   // temperaturas
    {10, 15, 20}       // durações
};

esp_err_t ConfigManager::init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;
    mutex_ = xSemaphoreCreateMutex();
    return (mutex_ != nullptr) ? ESP_OK : ESP_ERR_NO_MEM;
}

bool ConfigManager::hasDefaultConfig() {
    nvs_handle_t h;
    if (nvs_open("brew_cfg", NVS_READONLY, &h) != ESP_OK) return false;
    size_t sz = 0;
    esp_err_t err = nvs_get_blob(h, "default", nullptr, &sz);
    nvs_close(h);
    return (err == ESP_OK && sz == sizeof(BrewConfig));
}

esp_err_t ConfigManager::clearDefaultConfig() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
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

esp_err_t ConfigManager::saveToFlash() {
    // Monta BrewConfig a partir do array
    BrewConfig cfg;
    cfg.step_count = static_cast<uint8_t>(stepCount_);
    for (size_t i = 0; i < stepCount_; ++i) {
        cfg.temperatures[i] = temps_[i];
        cfg.durations[i]    = durations_[i];
    }
    // Zera o resto
    for (size_t i = stepCount_; i < MAX_STEPS; ++i) {
        cfg.temperatures[i] = 0.0f;
        cfg.durations[i]    = 0;
    }
    // Salva na NVS
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
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

esp_err_t ConfigManager::loadFromFlash() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(500)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
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

    // Atualiza RAM
    clearSteps();
    for (size_t i = 0; i < cfg.step_count; ++i) {
        pushStep(cfg.temperatures[i], cfg.durations[i]);
    }
    return ESP_OK;
}

esp_err_t ConfigManager::resetToFactory() {
    // Em RAM
    clearSteps();
    for (size_t i = 0; i < FACTORY_DEFAULT.step_count; ++i) {
        pushStep(FACTORY_DEFAULT.temperatures[i], FACTORY_DEFAULT.durations[i]);
    }
    // E na flash
    return saveToFlash();
}

//---------------- Array em RAM ----------------
size_t ConfigManager::getStepCount() {
    return stepCount_;
}

bool ConfigManager::pushStep(int temp, uint32_t duration) {
    if (stepCount_ >= MAX_STEPS) return false;
    temps_[stepCount_]     = temp;
    durations_[stepCount_] = duration;
    ++stepCount_;
    return true;
}

bool ConfigManager::popStep() {
    if (stepCount_ == 0) return false;
    --stepCount_;
    // opcional: limpa o slot
    temps_[stepCount_]     = 0;
    durations_[stepCount_] = 0;
    return true;
}

int ConfigManager::getTemperature(size_t idx) {
    return (idx < stepCount_) ? temps_[idx] : nanf("");
}

uint32_t ConfigManager::getDuration(size_t idx) {
    return (idx < stepCount_) ? durations_[idx] : 0;
}

void ConfigManager::clearSteps() {
    stepCount_ = 0;
    // opcional: zerar buffers
}

//---------------- Operations para o Statechart ----------------
void op_InitConfig() {
    esp_err_t err = ConfigManager::init();
    if (err == ESP_OK) uart_send("ConfigManager init OK\r\n");
    else             uart_send("ConfigManager init FAIL\r\n");
}

void op_LoadConfigFromFlash() {
    esp_err_t err = ConfigManager::loadFromFlash();
    if (err == ESP_OK) {
        uart_send("Config carregada da flash\r\n");
        fireEvent(STATECHART, EVENT_CFG_LOADED);
    } else {
        uart_send("Falha ao carregar config da flash\r\n");
        fireEvent(STATECHART, EVENT_CFG_ERROR);
    }
}

void op_SaveConfigToFlash() {
    esp_err_t err = ConfigManager::saveToFlash();
    if (err == ESP_OK) uart_send("Config salva na flash\r\n");
    else                uart_send("Erro ao salvar config na flash\r\n");
}

void op_ClearFlashConfig() {
    esp_err_t err = ConfigManager::clearDefaultConfig();
    if (err == ESP_OK) uart_send("Config da flash limpa\r\n");
    else                uart_send("Erro ao limpar config da flash\r\n");
}

void op_ResetToFactory() {
    esp_err_t err = ConfigManager::resetToFactory();
    if (err == ESP_OK) {
        uart_send("Factory reset completo\r\n");
        fireEvent(STATECHART, EVENT_CFG_LOADED);
    } else {
        uart_send("Erro no factory reset\r\n");
        fireEvent(STATECHART, EVENT_CFG_ERROR);
    }
}
