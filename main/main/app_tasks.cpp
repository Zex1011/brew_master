#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "Statechart.h"
#include "CallbackModule.hpp"
#include <cmath>
#include <stdint.h>
// <<< PID – inclui biblioteca -----------------------------
#include <PID_v1.h>                    // biblioteca oficial do Arduino
// <<< END PID --------------------------------------------


// Testando aplicação de I2C
#include <Wire.h>          // importante

/* Endereços I²C - ajuste conforme seus sensores */
constexpr uint8_t I2C_ADDR_SENSOR1 = 0x08; //teste
constexpr uint8_t I2C_ADDR_SENSOR2 = 0x09;
//constexpr uint8_t I2C_ADDR_SENSOR1 = 0x48;
//constexpr uint8_t I2C_ADDR_SENSOR2 = 0x49;


////
// Testando aplicação de I2C
static int16_t readTemp8(uint8_t addr)
{
    Wire.beginTransmission(addr);
    //Wire.write(0x00);                // registrador/ponteiro a ler (exemplo)
    //Wire.endTransmission(false);     // *false* mantém o bus ativo (repeated-start)

    Wire.requestFrom(addr, (uint8_t)1);
    if (Wire.available() < 2) return INT8_MIN;   // erro

    uint8_t msb = Wire.read();
    //uint8_t lsb = Wire.read();
    //return (int16_t)((msb << 8) | lsb);           // formato big-endian, 16 bits
    return msb;
}
////

volatile int8_t g_sensor1 = 20;   // temperatura inicial fictícia
volatile int8_t g_sensor2 = 20;

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

// ---------------------------------------------------------------------------
//                              PID CONTROL TASK
// ---------------------------------------------------------------------------
// <<< PID – configuração de PWM/LEDC e controlador --------------------------
constexpr uint8_t  PWM_PIN       = 2;      // pino de saída PWM
constexpr int PWM_FREQUENCY = 1000;   // 19,5 kHz
constexpr int  PWM_RES_BITS  = 10;      // 0‑1023
constexpr uint16_t PWM_MAX_DUTY  = (1u << PWM_RES_BITS) - 1;



// ganhos do controlador
constexpr double Kp = 20.0;
constexpr double Ki = 0.1;
constexpr double Kd = 5.0;

// variáveis do PID
double pidInput  = 0.0;
double pidOutput = 0.0;
double pidSetPt  = 0.0;
PID    pid(&pidInput, &pidOutput, &pidSetPt, Kp, Ki, Kd, DIRECT);

// Task propriamente dita
static void PidTask(void*)
{   
      // Configura PWM (checando erro)
    if (!ledcAttach(PWM_PIN, PWM_FREQUENCY, PWM_RES_BITS))
        Serial.println("Falha no LEDC!");
    
    // ------- PID: inicialização única -------
    pid.SetOutputLimits(0, PWM_MAX_DUTY); // 0‑1023
    pid.SetSampleTime(10);                // 10 ms = 100 Hz
    pid.SetMode(AUTOMATIC);               // liga o controlador


    const TickType_t period = pdMS_TO_TICKS(10);   // 100 Hz
    TickType_t lastWake    = xTaskGetTickCount();

    for (;;)
    {
        // --- lê variáveis compartilhadas (área crítica curta) ------------
        int16_t pid_sp, pid_pv;
        //taskENTER_CRITICAL();
        pid_sp = cb.setPoint;
        pid_pv = g_sensor1;
        //taskEXIT_CRITICAL();

        // atualiza entradas do PID
        pidSetPt = static_cast<double>(pid_sp);
        pidInput = static_cast<double>(pid_pv);
        pid.Compute();

        // escreve PWM (já limitado por SetOutputLimits)
        uint16_t duty = constrain(static_cast<int>(pidOutput), 0, PWM_MAX_DUTY);
        //uint16_t duty = 500;

        //Serial.printf("duty=%u\n", duty);
        ledcWrite(PWM_PIN, duty);

        // espera próximo ciclo
        vTaskDelayUntil(&lastWake, period);
    }
}
// <<< END PID ----------------------------------------------------------------




//I2C task
static void I2CTask(void*)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));          // 1 s (mesmo período da TempTask)

        int8_t t1 = readTemp8(I2C_ADDR_SENSOR1);
        int8_t t2 = readTemp8(I2C_ADDR_SENSOR2);

        if (t1 != INT8_MIN) g_sensor1 = t1;      // atualiza só se leitura OK
        if (t2 != INT8_MIN) g_sensor2 = t2;
    }
}
////



/* ---------- TemperatureTask ---------- */
static void TempTask(void *) {

    bool mixerOn = false;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        int8_t t1 = g_sensor1;
        int8_t t2 = g_sensor2;
        int8_t sp = cb.setPoint;
        /* --- Timer_counter: baseado no sensor1 --- */
        bool temp_wrong = (t1-sp)>1;
        withSM([&]{
            if (temp_wrong)     machine.raiseTemp_wrong();
            else                machine.raiseTemp_right();
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

    xTaskCreate(I2CTask, "i2c", 4096, NULL, 4, NULL);
    ///
    xTaskCreate(TimerTask     , "timer", 2048, NULL, 5, NULL);
    xTaskCreate(TempTask      , "temp" , 4096, NULL, 4, NULL);

    xTaskCreate(PidTask       , "pid"  , 4096, NULL, 4, NULL);   // <<< PID task

    xTaskCreate(UartTask      , "uart" , 2048, NULL, 3, NULL);
}
