#include <Wire.h>

constexpr uint8_t  I2C_SLAVE_ADDR = 0x08;    // Endereço I2C do ESP32
constexpr int      SDA_PIN        = 21;      // Pino SDA padrão do ESP32
constexpr int      SCL_PIN        = 22;      // Pino SCL padrão do ESP32
constexpr uint32_t I2C_FREQ       = 100000;  // Frequência do barramento (100kHz)

volatile uint16_t randomValue = 42;          // Valor a ser enviado ao master

// Callback: master requisitou dados
void onRequest() {
  Wire.write(highByte(randomValue));         // Envia byte alto
  Wire.write(lowByte(randomValue));          // Envia byte baixo
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Inicializando...");

  randomSeed(micros());                      // Semente aleatória

  // Inicializa o ESP32 como slave I2C (4 parâmetros obrigatórios!)
  bool ok = Wire.begin(I2C_SLAVE_ADDR, SDA_PIN, SCL_PIN, I2C_FREQ);
  if (!ok) {
    Serial.println("Erro ao iniciar como slave I2C!");
    while (true) delay(1000);  // trava aqui se falhar
  }

  Wire.onRequest(onRequest);                 // Define o callback
  Serial.println("Pronto como I2C slave");
}

void loop() {
  // Atualiza valor aleatório entre 25 e 100 a cada 0.5s
  randomValue = random(25, 101);
  delay(500);
}
