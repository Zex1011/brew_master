#include <Arduino.h>           // APIs pinMode / digitalWrite
#include "GPIO_Module.hpp"

void GPIO_Module::initPin(sc::integer pin)
{
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pin), LOW);   // inicia desligado
}

void GPIO_Module::writePin(sc::integer pin, sc::integer value)
{
    digitalWrite(static_cast<uint8_t>(pin), value ? HIGH : LOW);
}
