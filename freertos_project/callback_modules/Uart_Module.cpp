#include "Uart_Module.hpp"


void UartModule::configUART () {
  Serial.begin(9600);
  delay(10);
}

//void CallbackModule::writePin(sc::integer pin, sc::integer value) {
//  //Serial.print("[DEBUG] writePin: pin=");
//  //Serial.print(pin);
//  //Serial.print(" value=");
//  //Serial.println(value);
//  digitalWrite(pin, value ? HIGH : LOW);
//}

void CallbackModule::writeUart(std::string message) {
  Serial.print(message.c_str());
}