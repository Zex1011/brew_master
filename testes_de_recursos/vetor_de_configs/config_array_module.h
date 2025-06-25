// config_array_module.h
#ifndef CONFIG_ARRAY_MODULE_H
#define CONFIG_ARRAY_MODULE_H

#include <cstdint>
#include <cstddef>
#include <cmath>  // para nanf()

/**
 * @brief Gerencia um conjunto de etapas de brassagem (temperatura + duração).
 *
 * Permite adicionar, consultar, remover a última etapa (undo) e limpar todas as etapas.
 */
class ConfigArray {
public:
    /// Número máximo de etapas suportadas
    static constexpr size_t MAX_STEPS = 20;

    /**
     * @brief Obtém a quantidade de etapas atualmente configuradas.
     * @return Número de etapas (0 a MAX_STEPS).
     */
    static size_t getStepCount();

    /**
     * @brief Adiciona uma nova etapa.
     * @param temp      Temperatura em °C.
     * @param duration  Duração em segundos.
     * @return true se adicionada com sucesso, false em caso de overflow.
     */
    static bool pushStep(float temp, uint32_t duration);

    /**
     * @brief Lê a temperatura de uma etapa sem removê-la.
     * @param idx Índice da etapa (0 a getStepCount()-1).
     * @return Temperatura em °C ou nanf() se índice inválido.
     */
    static float getTemperature(size_t idx);

    /**
     * @brief Lê a duração de uma etapa sem removê-la.
     * @param idx Índice da etapa (0 a getStepCount()-1).
     * @return Duração em segundos ou UINT32_MAX se índice inválido.
     */
    static uint32_t getDuration(size_t idx);

    /**
     * @brief Remove a última etapa configurada (undo).
     * @return true se havia uma etapa para remover, false se vazio.
     */
    static bool popStep();

    /**
     * @brief Limpa todas as etapas configuradas.
     */
    static void clear();

private:
    /// Array de temperaturas (°C)
    static float    temps_[MAX_STEPS];
    /// Array de durações (s)
    static uint32_t durations_[MAX_STEPS];
    /// Contador de etapas válidas
    static size_t   stepCount_;
};

#endif // CONFIG_ARRAY_MODULE_H
