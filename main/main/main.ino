#include <Arduino.h>
#include "app_tasks.hpp"   // declare app_tasks_init()

void setup()
{
    app_tasks_init();      // inicializa tudo, cria as tasks
}

void loop()
{
    // vazio – FreeRTOS está executando as tasks
}
