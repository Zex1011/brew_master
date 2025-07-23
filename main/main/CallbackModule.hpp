//  main/CallbackModule.hpp
#include <Arduino.h>
#pragma once
#include "Statechart.h"
#include "ConfigManager.h"
#include "GPIO_Module.hpp"
#include "Uart_Module.hpp"
//#include "driver/gpio.h"

// ajuste se seus pinos estiverem em outro header
constexpr gpio_num_t PIN_HEATER = GPIO_NUM_18;
constexpr gpio_num_t PIN_MIXER  = GPIO_NUM_5;

/**
 * Implementa TODAS as operações exigidas por Statechart::OperationCallback.
 * Qualquer método que você ainda não queira usar agora pode ficar vazio.
 */
class CallbackModule : public Statechart::OperationCallback {
public:
    /* ---- inicialização de hardware ---- */
    void configUART() override;
    void configGPIO() override;

    /* ---- controle físico ---- */
    //void writeHeater(sc::integer val) override;
    void writeMixer (sc::integer val) override;


    /* ---- escrita UART ---- */
    void writeUartString(std::string msg) override;
    void writeUartInt(sc::integer v)      override;

    /* ---- UART inteiro recebido ---- */
    sc::integer op_getUartInt() override;

    /* ---- persistência / curvas ---- */
    void op_InitConfig()            override;
    void op_LoadConfigFromFlash()   override;
    void op_SaveConfigToFlash()     override;
    void op_ClearFlashConfig()      override;
    void op_ResetToFactory()        override;


    // Config array ---------------------------------------------------------
    void op_PushStep(sc::integer t, sc::integer d) override;
    void op_PopStep()                      override;
    void op_ClearSteps()                     override;
    sc::integer op_GetStepCount()            override;
    sc::integer op_GetTemperature(sc::integer i) override;
    sc::integer op_GetDuration   (sc::integer i) override;
    void op_PrintConfig()                    override;

    /* ---- cronômetro ---- */
    void op_TimerInit()                 override;
    void op_StartTimer(sc::integer s)   override;
    void op_StopTimer()                 override;
    void op_ContinueTimer()             override;
    bool op_IsTimerRunning()            override;

    /* ---- set-point ---- */
    sc::integer op_SetTemperature(sc::integer idx) override;

    /* ---- variáveis compartilhadas com as tasks ---- */
    int32_t lastUartInt = 0;
    bool    timerRunning = false;
    int32_t secLeft      = 0;
    volatile int setPoint = 0;   // <-- TODO verificar volatile
};
