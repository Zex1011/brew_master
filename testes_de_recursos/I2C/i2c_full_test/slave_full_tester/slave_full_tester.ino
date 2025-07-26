#include <Wire.h>

// Definir o endereço I2C para este ESP32 (deve ser o mesmo que o Mestre usa)
constexpr uint8_t I2C_SLAVE_ADDRESS = 0x08; 

// Pinos I2C do ESP32 (SDA, SCL)
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// Frequência do barramento I2C (100 kHz é um bom padrão)
constexpr uint32_t I2C_FREQ = 100000; 

// Pino ADC para receber o sinal do PWM do mestre (ex: GPIO34)
const int ADC_INPUT_PIN = 34;

// Valores de simulação de temperatura e ADC
const float MIN_TEMP_SIMULATED = 25.0; // Corresponde a 0% PWM / 0V
const float MAX_TEMP_SIMULATED = 100.0; // Corresponde a 100% PWM / 3.3V

// A resolução do ADC do ESP32 é 12 bits por padrão (0-4095)
const int ADC_MAX_VALUE = 4095; // Para 12 bits de resolução

volatile float simulatedTemperature = MIN_TEMP_SIMULATED; // Temperatura inicial simulada

unsigned long last_simulated_temp_update_time; // Para calcular o delta t

// Parâmetros do modelo de inércia térmica
const float TEMPERATURA_AMBIENTE = 25.0; // Temperatura que o tanque tende se não aquecido - 25C
const float GANHO_AQUECIMENTO = 1.0;    // Quão eficientemente o PWM aquece (graus C por segundo por unidade de PWM) - 0.05
const float TAXA_RESFRIAMENTO = 0.05;    // Quão rápido o tanque resfria para a temperatura ambiente (fator) - 0.01


// Callback: master requisitou dados
void onRequest() {
  float temp_copy = simulatedTemperature; 
  Serial.printf("RequestEvent: Mestre requisitou dados. Enviando temperatura: %.2f C\n", temp_copy);
  byte data[4];
  memcpy(data, &temp_copy, 4); 
  Wire.write(data, 4); 
}

// Callback: master enviou dados (debug)
void onReceive(int howMany) {
  Serial.printf("ReceiveEvent: Mestre enviou %d bytes. ", howMany);
  while (Wire.available()) {
    byte c = Wire.read(); 
    Serial.print((char)c);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200); 
  Serial.println("\nESP32 I2C Slave Simulator - Iniciando...");

  // Configura a resolução do ADC
  analogReadResolution(12); // Define a resolução do ADC para 12 bits (0-4095)

  // Inicializa o ESP32 como slave I2C
  bool ok = Wire.begin(I2C_SLAVE_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  if (!ok) {
    Serial.println("Erro ao iniciar como slave I2C! Travando...");
    while (true) delay(1000);
  }

  Wire.onRequest(onRequest);    
  Wire.onReceive(onReceive);    
  
  Serial.println("Pronto como I2C slave no endereco 0x08.");
  Serial.printf("Pinos I2C: SDA=%d, SCL=%d. Frequencia: %d Hz.\n", I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Serial.printf("Pino ADC de entrada para simular aquecimento: GPIO%d\n", ADC_INPUT_PIN);

  last_simulated_temp_update_time = millis(); // Inicializa o tempo para cálculo do dt
}

void loop() {
  unsigned long current_time = millis();
  float dt_seconds = (float)(current_time - last_simulated_temp_update_time) / 1000.0; // Tempo em segundos
  last_simulated_temp_update_time = current_time;


  // Lê a potência do aquecedor (do PWM do mestre)
  int adc_value = analogRead(ADC_INPUT_PIN);
  float heating_power_0_to_1 = (float)adc_value / ADC_MAX_VALUE; // Converte ADC para escala 0.0 a 1.0

  // Calcula a variação da temperatura (modelo de inércia)
  float delta_T = 0;
  // Efeito do Aquecimento: Direto da potência do PWM
  // heating_power_0_to_1 * GANHO_AQUECIMENTO => representa a taxa de aquecimento instantânea em C/s
  delta_T += heating_power_0_to_1 * GANHO_AQUECIMENTO * dt_seconds;

  // Efeito do Resfriamento para o Ambiente: Se o tanque está mais quente/frio que o ambiente, ele tende ao ambiente
  delta_T -= (simulatedTemperature - TEMPERATURA_AMBIENTE) * TAXA_RESFRIAMENTO * dt_seconds;
  
  // Atualiza a temperatura simulada
  simulatedTemperature += delta_T;

  // Limita a temperatura simulada aos limites (bom para evitar valores absurdos)
  if (simulatedTemperature < MIN_TEMP_SIMULATED) simulatedTemperature = MIN_TEMP_SIMULATED;
  if (simulatedTemperature > MAX_TEMP_SIMULATED) simulatedTemperature = MAX_TEMP_SIMULATED; 

  // Imprimir a temperatura simulada para depuração no escravo
  Serial.printf("Slave: ADC = %d (P=%.2f), dT=%.3f, Temp=%.2f C\n", adc_value, heating_power_0_to_1, delta_T, simulatedTemperature);
  
  delay(100); // Roda o loop a cada 100ms
}