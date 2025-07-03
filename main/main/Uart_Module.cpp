#include <Arduino.h>
#include "Uart_Module.hpp"

void UartModule::configUART(uint32_t baud)
{
    Serial.begin(baud);
    while (!Serial) { vTaskDelay(1); }   // espera USB-CDC enumerar
}

void UartModule::writeUart(const std::string& msg)
{
    Serial.print(msg.c_str());
}

void UartModule::writeUartInt(int32_t value)
{
    Serial.println(value);               // println já põe \r\n
}
