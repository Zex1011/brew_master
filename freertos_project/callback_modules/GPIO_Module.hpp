#ifndef GPIO_MODULE_HPP
#define GPIO_MODULE_HPP

#include "Statechart.h"
#include <Arduino.h>

class GPIO_Module : public Statechart::OperationCallback {
public:
  void writePin(sc::integer pin, sc::integer value) override;
};

#endif
