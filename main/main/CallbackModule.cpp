//  main/CallbackModule.cpp
#include <Arduino.h>
#include "CallbackModule.hpp"
#include <cmath>

/* ---------- INIT ---------- */
void CallbackModule::configUART() { UartModule::configUART(); }

void CallbackModule::configGPIO() {
    GPIO_Module::initPin(PIN_HEATER);
    GPIO_Module::initPin(PIN_MIXER);
}

/* ---------- controle físico ---------- */
//void CallbackModule::writeHeater(sc::integer val) // obsoleto com o PID implementado
//{
//    GPIO_Module::writePin(PIN_HEATER, val);
//}


void CallbackModule::writeMixer(sc::integer val)
{
    GPIO_Module::writePin(PIN_MIXER, val);
}

/* ---------- UART helpers ---------- */
void CallbackModule::writeUartString(std::string msg) { UartModule::writeUart(msg); }
void CallbackModule::writeUartInt(sc::integer v)      { UartModule::writeUartInt(v); }

sc::integer CallbackModule::op_getUartInt() { return lastUartInt; }

/* ---------- ConfigManager wrappers ---------- */
void CallbackModule::op_InitConfig()          { ConfigManager::init(); }
void CallbackModule::op_LoadConfigFromFlash() { ConfigManager::loadFromFlash(); }
void CallbackModule::op_SaveConfigToFlash()   { ConfigManager::saveToFlash(); }
void CallbackModule::op_ClearFlashConfig()    { ConfigManager::clearDefaultConfig(); }
void CallbackModule::op_ResetToFactory()      { ConfigManager::resetToFactory(); }

void CallbackModule::op_PushStep(sc::integer t, sc::integer d) { ConfigManager::op_PushStep(t, d); }
void CallbackModule::op_PopStep()                              { ConfigManager::op_PopStep(); }
void CallbackModule::op_ClearSteps()                           { ConfigManager::clearSteps(); }
sc::integer CallbackModule::op_GetStepCount()                  { return ConfigManager::getStepCount(); }
sc::integer CallbackModule::op_GetTemperature(sc::integer i)   { return ConfigManager::getTemperature(i); }
sc::integer CallbackModule::op_GetDuration   (sc::integer i)   { return ConfigManager::getDuration(i); }
void CallbackModule::op_PrintConfig()                          { ConfigManager::printConfig(); }

/* ---------- cronômetro ---------- */
void CallbackModule::op_TimerInit()               { timerRunning = false; secLeft = 0; }
void CallbackModule::op_StartTimer(sc::integer s) { timerRunning = s > 0; secLeft = s; }
void CallbackModule::op_StopTimer()               {timerRunning = false; }
void CallbackModule::op_ContinueTimer()           {timerRunning = false; }
bool CallbackModule::op_IsTimerRunning()          { return timerRunning; }

/* ---------- set-point ---------- */
//sc::integer CallbackModule::op_SetTemperature(sc::integer idx) {
//    if (idx < ConfigManager::getStepCount())
//        setPoint = ConfigManager::getTemperature(idx);
//        Serial.print("DEBUG setPoint setado");
//    else
//        Serial.print("DEBUG problema com stepcount");
//    return setPoint;
//}
sc::integer CallbackModule::op_SetTemperature(sc::integer value)
{
    setPoint = value;            // write já protegida pelo withSM()
    return setPoint;
}
