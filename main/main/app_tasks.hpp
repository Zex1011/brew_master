/*  app_tasks.hpp
 *  -------------------------------------------------------------
 *  Declara a função de bootstrap das tasks FreeRTOS
 *  e expõe (opcionalmente) os objetos globais.
 */
#ifndef APP_TASKS_HPP
#define APP_TASKS_HPP

#include "Statechart.h"
#include "CallbackModule.hpp"

/*  Objetos globais criados em app_tasks.cpp  ------------------ */
extern Statechart       machine;
extern CallbackModule   cb;

/*  Inicializa GPIO, UART, máquina de estados e cria as tasks.  */
void app_tasks_init();

#endif /* APP_TASKS_HPP */
