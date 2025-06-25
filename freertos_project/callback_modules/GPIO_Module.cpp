#include "GPIO_Module.hpp"

void GPIO_Module::writePin(sc::integer pin, sc::integer value) {
  Serial.print("[DEBUG] writePin: pin=");
  Serial.print(pin);
  Serial.print(" value=");
  Serial.println(value);
  digitalWrite(pin, value ? HIGH : LOW);
}
