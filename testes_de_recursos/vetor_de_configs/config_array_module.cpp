
// config_array_module.cpp
#include "config_array_module.h"

// Definições dos membros estáticos
float    ConfigArray::temps_[ConfigArray::MAX_STEPS]     = {0.0f};
uint32_t ConfigArray::durations_[ConfigArray::MAX_STEPS] = {0};
size_t   ConfigArray::stepCount_                         = 0;

size_t ConfigArray::getStepCount() {
    return stepCount_;
}

bool ConfigArray::pushStep(float temp, uint32_t duration) {
    if (stepCount_ >= MAX_STEPS) {
        return false;  // overflow: limite atingido
    }
    temps_[stepCount_]     = temp;
    durations_[stepCount_] = duration;
    ++stepCount_;
    return true;
}

float ConfigArray::getTemperature(size_t idx) {
    if (idx < stepCount_) {
        return temps_[idx];
    }
    return nanf(""); // índice inválido
}

uint32_t ConfigArray::getDuration(size_t idx) {
    if (idx < stepCount_) {
        return durations_[idx];
    }
    return 0;  // índice inválido
}

bool ConfigArray::popStep() {
    if (stepCount_ == 0) {
        return false;  // underflow: nada para remover
    }
    --stepCount_;
    return true;
}

void ConfigArray::clear() {
    stepCount_ = 0;
    // para segurança, poderia zerar arrays, mas não é estritamente necessário
}
