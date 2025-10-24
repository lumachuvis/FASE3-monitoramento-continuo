// TESTE 1 — Serial + Blink
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("TESTE 1: Serial OK");
  pinMode(2, OUTPUT); // LED on-board (às vezes não aparece, mas pisca)
}

void loop() {
  static bool s;
  s = !s;
  digitalWrite(2, s);
  Serial.println(millis());
  delay(500);
}
