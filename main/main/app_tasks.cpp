#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "Statechart.h"
#include "CallbackModule.hpp"
#include <cmath>

#include <stdint.h>

// Testando aplicação de I2C
#include <Wire.h>          // ⬅ importante

/* Endereços I²C - ajuste conforme seus sensores */
constexpr uint8_t I2C_ADDR_SENSOR1 = 0x08; //teste
//constexpr uint8_t I2C_ADDR_SENSOR1 = 0x48;
//constexpr uint8_t I2C_ADDR_SENSOR2 = 0x49;

////
// Testando aplicação de I2C
static int16_t readTemp16(uint8_t addr)
{
    Wire.beginTransmission(addr);
    Wire.write(0x00);                // registrador/ponteiro a ler (exemplo)
    Wire.endTransmission(false);     // *false* mantém o bus ativo (repeated-start)

    Wire.requestFrom(addr, (uint8_t)2);
    if (Wire.available() < 2) return INT16_MIN;   // erro

    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    return (int16_t)((msb << 8) | lsb);           // formato big-endian, 16 bits
}
////

volatile int16_t g_sensor1 = 20;   // temperatura inicial fictícia
volatile int16_t g_sensor2 = 20;

static Statechart     machine;
static CallbackModule cb;
static SemaphoreHandle_t smMtx;

template<typename F>
void withSM(F&& fn){
    xSemaphoreTake(smMtx, portMAX_DELAY);
    fn();
    machine.triggerWithoutEvent();
    xSemaphoreGive(smMtx);
}
/* ---------- TimerTask ---------- */
static void TimerTask(void*){
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(1000));
        if(cb.timerRunning && --cb.secLeft==0){
            cb.timerRunning=false;
            withSM([&]{ machine.raiseTimer_trigger(); });
        }
    }
}

//I2C task
static void I2CTask(void*)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));          // 1 s (mesmo período da TempTask)

        int16_t t1 = readTemp16(I2C_ADDR_SENSOR1);
        //int16_t t2 = readTemp16(I2C_ADDR_SENSOR2);

        if (t1 != INT16_MIN) g_sensor1 = t1;      // atualiza só se leitura OK
        //if (t2 != INT16_MIN) g_sensor2 = t2;
    }
}
////



/* ---------- TemperatureTask ---------- */
// TODO IMPLEMENTAR O CONTROLADOR PARA ATIVAR E DESATIVAR UM EVENTO DE "FORA DE TEMPERATURA"
// PODERÁ USAR A INFRAESTRUTURA ATUAL DE "HEATER ON" E "HEATER OFF", E NO ESTADO DE "FORA DE TEMPERATURA"
// ELE ATIVARIA A FUNÇÃO "STOP TIMER" ASSIM APENAS CONTANDO O TEMPO ENQUANTO O ERRO DO CONTROLADOR FOR
// ACEITAVEL
static void TempTask(void *) {

    bool mixerOn = false;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        int16_t t1 = g_sensor1;
        int16_t t2 = g_sensor2;
        int16_t sp = cb.setPoint;
        /* --- Heater: baseado no sensor1 --- */
        bool below = (t1 < sp - 1);
        withSM([&]{
            if (below)  machine.raiseHeater_on();
            else        machine.raiseHeater_off();
        });

        /* --- Mixer: diferença entre sensores --- */
        bool diff = (std::abs(t1 - t2) > 1);

        if (diff && !mixerOn) {
            mixerOn = true;
            withSM([&]{ machine.raiseMixer_on(); });
        }
        if (!diff && mixerOn) {
            mixerOn = false;
            withSM([&]{ machine.raiseMixer_off(); });
        }

        /* Log • Ex.: DATA-setP-s1-s2-diffFlag */
        printf("DATA-%d-%d-%d-%d\n",
               sp, t1, t2, diff ? 1 : 0);
    }
}

static void UartTask(void*) {
    constexpr size_t BUF_MAX = 32;
    char buf[BUF_MAX];
    size_t idx = 0;

    for (;;) {
        // lê tudo que chegou
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                buf[idx] = 0;  // fecha string

                // --- tratar linha inteira em buf ---
                // 1) é um inteiro puro?
                bool isInt = true;
                for (size_t i = 0; i < idx; ++i) {
                    if (!(buf[i] >= '0' && buf[i] <= '9') && !(i == 0 && buf[i]=='-')) {
                        isInt = false;
                        break;
                    }
                }
                if (isInt && idx > 0) {
                    cb.lastUartInt = atoi(buf);
                    withSM([&]{ machine.raiseInt_received(); });
                }
                else if (strcmp(buf, "start") == 0) {
                    withSM([&]{ machine.raiseStart_program(); });
                }
                else if (strcmp(buf, "default") == 0) {
                    withSM([&]{ machine.raiseUse_default(); });
                }
                else if (strcmp(buf, "reset") == 0) {
                    withSM([&]{ machine.raiseReset_default(); });
                }
                else if (strcmp(buf, "new") == 0) {
                    withSM([&]{ machine.raiseCreate_new(); });
                }
                else if (strcmp(buf, "cancel") == 0) {
                    withSM([&]{ machine.raiseCancel(); });
                }
                else if (strcmp(buf, "undo") == 0) {
                    withSM([&]{ machine.raiseUndo(); });
                }
                else if (strcmp(buf, "Add") == 0) {
                    withSM([&]{ machine.raiseAdd(); });
                }
                else if (strcmp(buf, "config") == 0) {
                    withSM([&]{ machine.raiseConfig(); });
                }
                else if (strcmp(buf, "ready") == 0) {
                    withSM([&]{ machine.raiseReady(); });
                }
                // 2) TEMPONExxx → g_sensor1
                else if (strncmp(buf, "TEMPONE", 7) == 0) {
                    g_sensor1 = atoi(buf + 7);
                }
                // 3) TEMPTWOxxx → g_sensor2
                else if (strncmp(buf, "TEMPTWO", 7) == 0) {
                    g_sensor2 = atoi(buf + 7);
                }
                // se quiser, pode logar o comando não reconhecido:
                // else Serial.printf("CMD unknown: %s\n", buf);

                idx = 0;  // reinicia buffer pra próxima linha
            }
            else if (idx + 1 < BUF_MAX) {
                buf[idx++] = c;  // acumula caractere
            }
            // caso ultrapasse BUF_MAX, simplesmente segue e descarta excedente
        }

        // sem dado, espera um pouco antes de tentar de novo
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// antes era extern "C" void app_main()
void app_tasks_init()          // novo nome
{
    //Serial.begin(9600);
    smMtx = xSemaphoreCreateMutex();

    cb.configGPIO();           // se usar pinMode/digitalWrite
    cb.configUART();           // se ainda quiser

    machine.setOperationCallback(&cb);
    machine.enter();
    /// I2C
    Wire.begin();                     // inicia I²C com pinos padrão (SDA21/SCL22)

    xTaskCreate(I2CTask, "i2c", 4096, NULL, 4, NULL);   // nova task
    ///
    xTaskCreate(TimerTask     , "timer", 2048, NULL, 5, NULL);
    xTaskCreate(TempTask      , "temp" , 4096, NULL, 4, NULL);
    xTaskCreate(UartTask      , "uart" , 2048, NULL, 3, NULL);
}
