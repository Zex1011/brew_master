#ifndef UART_MODULE_HPP
#define UART_MODULE_HPP

#include "Statechart.h"
#include <Arduino.h>

class UartModule : public Statechart::OperationCallback {
public:
void configUart();
void writeUart(std::string message);
//void writePin(sc::integer pin, sc::integer value) override;
};

#endif
