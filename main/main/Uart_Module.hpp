#pragma once
#include <Arduino.h>
#include <string>

class UartModule {
public:
    static void configUART(uint32_t baud = 9600);   // ⬅ default
    static void writeUart(const std::string& msg);
    static void writeUartInt(int32_t value);
};
