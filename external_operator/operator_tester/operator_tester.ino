void setup() {
  Serial.begin(115200);
  delay(1000); // Espera a serial estabilizar

  Serial.println("LOG-Inicializando teste do ESP32");
}

void loop() {
  // Simula dados
  static int count = 0;
  int y = random(50, 100);     // Y - gráfico 1
  int z = random(100, 150);    // Z - gráfico 2
  bool k = (count % 2 == 0);   // K - gráfico 3 alternando 0/1

  // Envia dado no formato DATA
  Serial.print("DATA-");
  Serial.print(y);
  Serial.print("-");
  Serial.print(z);
  Serial.print("-");
  Serial.println(k ? "1" : "0");

  // Envia logs periódicos
  if (count % 5 == 0) {
    Serial.println("LOG-Pacote enviado com sucesso");
  }

  // Envia um erro de teste a cada 13 ciclos
  if (count % 13 == 0) {
    Serial.println("ERROR-Teste de erro gerado pelo ESP");
  }

  count++;
  delay(1000); // Envia a cada 1 segundo
}
