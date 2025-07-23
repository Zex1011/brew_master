const uint8_t LED_PIN = 2;      // ajuste aqui
const uint8_t RES     = 10;
const uint32_t FREQ   = 1000;   // comece com 1 kHz

void setup() {
  Serial.begin(115200);
  bool ok = ledcAttach(LED_PIN, FREQ, RES);
  Serial.println(ok ? "LEDC OK" : "Falhou!");
}

void loop() {
  static uint32_t duty = 0;
  if (!ledcWrite(LED_PIN, duty)) Serial.println("Erro em ledcWrite");
  duty = (duty + 64) % (1u << RES);   // rampa lenta
  delay(50);
}
