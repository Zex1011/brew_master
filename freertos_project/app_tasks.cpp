// app_tasks.cpp – Tarefas FreeRTOS com statechart thread‑safe (C++ linkage)
// ----------------------------------------------------------------------
//  • Cada tarefa levanta evento diretamente; automato protegido por mutex
//  • TimerTask implementa cronômetro via operações op_Timer*             
//-----------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "Statechart.h"
#include "ConfigManager.h"
#include "uart.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "callback_modules/config_array_module.h"
#include "callback_modules/ConfigManager.h"
#include "callback_modules/ConfigManager.h"


//-----------------------------------------------------------------------
// 1. Instância única do automato + mutex (exclusão mútua)
//-----------------------------------------------------------------------
static BrewingStatemachine sc;
static SemaphoreHandle_t   scMutex = nullptr;
inline void sc_lock()   { xSemaphoreTake(scMutex, portMAX_DELAY); }
inline void sc_unlock() { xSemaphoreGive(scMutex); }

static void raiseEvent(uint16_t id) {
    sc_lock();
    auto* in = sc.getSCI_IN();
    switch (id) {
        case EVENT_START_PROGRAM:   in->raise_start_program();   break;
        case EVENT_USE_DEFAULT:     in->raise_use_default();     break;
        case EVENT_RESET_DEFAULT:   in->raise_reset_default();   break;
        case EVENT_CREATE_NEW:      in->raise_create_new();      break;
        case EVENT_CANCEL:          in->raise_cancel();          break;
        case EVENT_UNDO:            in->raise_undo();            break;
        case EVENT_ADD:             in->raise_add();             break;
        case EVENT_CONFIG:          in->raise_config();          break;
        case EVENT_READY:           in->raise_ready();           break;
        case EVENT_INT_RECEIVED:    in->raise_int_received();    break;
        case EVENT_TIMER_TRIGGER:   in->raise_timer_trigger();   break;
        case EVENT_HEATER_ON:       in->raise_heater_on();       break;
        case EVENT_HEATER_OFF:      in->raise_heater_off();      break;
        case EVENT_MIXER_ON:        in->raise_mixer_on();        break;
        case EVENT_MIXER_OFF:       in->raise_mixer_off();       break;
    }
    sc.runCycle();
    sc_unlock();
}

//-----------------------------------------------------------------------
// 2. UART – último inteiro recebido + operation op_getUartInt()
//-----------------------------------------------------------------------
static int32_t g_lastUartInt = 0;
int32_t op_getUartInt() { return g_lastUartInt; }

//-----------------------------------------------------------------------
// 3. TimerTask – cronômetro controlado por operations
//-----------------------------------------------------------------------
static volatile uint32_t timerSecRemaining = 0;
static volatile bool     timerRunning      = false;

void op_TimerInit() {
    timerRunning = false;
    timerSecRemaining = 0;
}

void op_StartTimer(int32_t seconds) {
    if (seconds > 0) {
        timerSecRemaining = static_cast<uint32_t>(seconds);
        timerRunning      = true;
    } else {
        timerRunning = false;
    }
}

bool op_IsTimerRunning() {
    return timerRunning;
}

static void TimerTask(void* pv) {
    const TickType_t period = pdMS_TO_TICKS(1000); // 1 s
    for (;;) {
        vTaskDelay(period);
        if (timerRunning && timerSecRemaining > 0) {
            if (--timerSecRemaining == 0) {
                timerRunning = false;
                raiseEvent(EVENT_TIMER_TRIGGER);
            }
        }
    }
}

//-----------------------------------------------------------------------
// 4. TemperatureTask – sensores I²C
//-----------------------------------------------------------------------
static float currentSetpoint = NAN;
static bool  heaterOn = false;
static bool  mixerOn  = false;

void op_SetTemperature(int idx) {
    currentSetpoint = ConfigManager::getTemperature(idx);
}

static float readSensor(uint8_t addr) { return 25.0f; /* TODO: I²C real */ }

static void TemperatureTask(void* pv) {
    const TickType_t period = pdMS_TO_TICKS(1000);
    for (;;) {
        vTaskDelay(period);
        float t1 = readSensor(0x48);
        float t2 = readSensor(0x49);

        if (!std::isnan(currentSetpoint)) {
            if (t1 < currentSetpoint - 0.2f && !heaterOn) { heaterOn = true;  raiseEvent(EVENT_HEATER_ON); }
            if (t1 >= currentSetpoint       &&  heaterOn) { heaterOn = false; raiseEvent(EVENT_HEATER_OFF);}    }
        float diff = fabsf(t1 - t2);
        if (diff > 1.0f && !mixerOn) { mixerOn = true;  raiseEvent(EVENT_MIXER_ON); }
        if (diff <= 0.5f && mixerOn) { mixerOn = false; raiseEvent(EVENT_MIXER_OFF); }
    }
}

//-----------------------------------------------------------------------
// 5. UartTask – comandos ASCII → eventos
//-----------------------------------------------------------------------
struct CmdMap { const char* cmd; uint16_t evt; };
static constexpr CmdMap cmdTable[] = {
    {"START",  EVENT_START_PROGRAM},
    {"DEF",    EVENT_USE_DEFAULT},
    {"RESET",  EVENT_RESET_DEFAULT},
    {"NEW",    EVENT_CREATE_NEW},
    {"CANCEL", EVENT_CANCEL},
    {"UNDO",   EVENT_UNDO},
    {"ADD",    EVENT_ADD},
    {"CONFIG", EVENT_CONFIG},
    {"READY",  EVENT_READY},
    {nullptr,   0}
};

static void parseLine(char* line) {
    while(*line==' '||*line=='\t') ++line;
    size_t len=strlen(line);
    while(len && (line[len-1]=='\r'||line[len-1]=='\n'||line[len-1]==' ')) line[--len]='\0';
    if(!len) return;

    for(const CmdMap* c=cmdTable; c->cmd; ++c) {
        if(!strcasecmp(line,c->cmd)) { raiseEvent(c->evt); return; }
    }

    char* end; long v=strtol(line,&end,10);
    if(*end=='\0') {
        g_lastUartInt = static_cast<int32_t>(v);
        raiseEvent(EVENT_INT_RECEIVED);
    } else {
        uart_send("Comando desconhecido\r\n");
    }
}

static void UartTask(void* pv) {
    char buf[64]; size_t idx=0;
    for(;;) {
        if(uart_available()) {
            int c = uart_read();
            if(c=='\n'||c=='\r') { buf[idx]='\0'; parseLine(buf); idx=0; }
            else if(idx<sizeof(buf)-1) buf[idx++] = static_cast<char>(c);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

//-----------------------------------------------------------------------
// 6. app_main – ponto de entrada (mantém extern "C" para o ESP‑IDF)
//-----------------------------------------------------------------------
extern "C" void app_main() {
    scMutex = xSemaphoreCreateMutex();

    ConfigManager::init();

    sc.init();
    sc.enter();

    op_TimerInit(); // cronômetro zerado

    xTaskCreate(TimerTask,      "TimerTask",  2048, nullptr, 5, nullptr);
    xTaskCreate(TemperatureTask,"TempTask",   4096, nullptr, 4, nullptr);
    xTaskCreate(UartTask,       "UartTask",   4096, nullptr, 3, nullptr);
}
