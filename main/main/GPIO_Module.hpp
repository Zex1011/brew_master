#pragma once
#include <stdint.h>
#include "sc_types.h"

class GPIO_Module {
public:
    static void initPin(sc::integer pin);               // configura como saída
    static void writePin(sc::integer pin, sc::integer value); // nível 0/1
};

