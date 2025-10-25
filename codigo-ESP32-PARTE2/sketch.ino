/*************************************************
 * Projeto: CardioIA – Parte 2 (MQTT + Dashboard)
 * Placa:   ESP32 DevKit v1 (Wokwi)
 * Objetivo: Ler DHT22 + botão (BPM) + MPU6050,
 *           montar JSON compacto e publicar via MQTT
 *           no HiveMQ (broker público) para consumir
 *           no Node-RED.
 *
 * Conexões (mesmas da Parte 1):
 *   DHT22          -> GPIO14
 *   Botão (BPM)    -> GPIO25 (INPUT_PULLUP)
 *   Chave ON/OFF   -> GPIO26 (HIGH = online)
 *   LED de alerta  -> GPIO4
 *   MPU6050 (I2C)  -> SDA=21, SCL=22
 *
 * Formato de payload (Node-RED):
 *   { t: 39.6, h: 48.3, b: 120, o: true, a: false }
 *************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

// --- DHT (usei a lib "DHT sensor library") ---
#include <DHT.h>
#define DHTPIN   14
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- Pinos (mesmos da Parte 1) ---
#define BUTTON_PIN   25    // botão para contar batimentos (INPUT_PULLUP)
#define SWITCH_PIN   26    // chave ONLINE/OFFLINE (HIGH = online)
#define LED_PIN       4    // LED para alerta visual

// --- MPU6050 (movimento bem simples) ---
MPU6050 mpu(Wire);
const float ACC_THRESHOLD = 0.20f;  // ~0.2 g de variação já considero “movimento”

// --- Limiares de alerta (mantive os da Parte 1) ---
const int   MAX_BPM  = 120;
const float MAX_TEMP = 38.0f;

// ======= Wi-Fi (Wokwi) =======
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ======= MQTT (HiveMQ público – sem TLS) =======
const char* MQTT_HOST  = "broker.hivemq.com";
const int   MQTT_PORT  = 1883;
const char* MQTT_TOPIC = "cardioia/luana-grupo/telemetry";

// Cliente MQTT (sem TLS mesmo, pois é broker público)
WiFiClient      wifiClient;
PubSubClient    mqtt(wifiClient);

// ======= Janelas/tempos =======
const unsigned long SAMPLE_MS     = 1000;   // taxa de amostragem (1s)
const unsigned long PUBLISH_MS    = 3000;   // publica a cada 3s
const unsigned long BPM_WINDOW_MS = 10000;  // janela de 10s para virar BPM
unsigned long lastSample  = 0;
unsigned long lastPublish = 0;

// ======= BPM por botão (debounce e janela) =======
unsigned long inicioJanelaBPM = 0;
unsigned long lastDebounce    = 0;
const unsigned long DEBOUNCE_MS = 30;
int pulsos = 0;            // contagem de toques dentro da janela
int bpm    = 0;            // BPM calculado a cada 10s
int lastBtn = HIGH;        // para o debounce

// ======= Movimento (estado simples) =======
bool movimento = false;

// ===================================================
//               Funções auxiliares
// ===================================================

// Conecta no Wi-Fi do Wokwi 
void conectaWiFi() {
  Serial.print("Conectando ao WiFi ");
  Serial.print(WIFI_SSID);
  Serial.print(" ... ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    if (millis() - t0 > 15000) break; // não fica preso se algo travar
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Falha ao conectar (vou tentar MQTT mesmo assim).");
  }
}

// Faz/re-faz a conexão MQTT (porta 1883)
// Aqui eu gero um clientId único para evitar conflito
void conectaMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  while (!mqtt.connected()) {
    String clientId = "esp32-cardioia-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("Conectando ao broker MQTT ... ");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("Conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqtt.state());
      Serial.println(" (tento de novo em 2s)");
      delay(2000);
    }
  }
}

// Leitura do botão com debounce e janela de 10s para virar BPM
void updateBPM() {
  int reading = digitalRead(BUTTON_PIN);

  // detecção de borda com debounce
  if (reading != lastBtn) {
    lastDebounce = millis();
    lastBtn = reading;
  }

  // quando o botão está estável em LOW, conto um “pulso”
  if ((millis() - lastDebounce) > DEBOUNCE_MS && reading == LOW) {
    pulsos++;
    delay(200); // protejo de multi-clique no simulador
  }

  // a cada 10s fecho a janela e transformo pulsos em BPM
  if (millis() - inicioJanelaBPM >= BPM_WINDOW_MS) {
    bpm = pulsos * 6;  // 10s * 6 = 60s (BPM)
    pulsos = 0;
    inicioJanelaBPM = millis();
    Serial.print("[BPM] Batimentos: ");
    Serial.println(bpm);
  }
}

// Atualiza o estado “movimento” usando a aceleração
void updateMovimento() {
  mpu.update();
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  // Parada “ideal” ≈ (0, 0, ~1g). Se sair desse envelope, considero movimento.
  bool movX = fabs(ax) > ACC_THRESHOLD;
  bool movY = fabs(ay) > ACC_THRESHOLD;
  bool movZ = fabs(az - 1.0f) > ACC_THRESHOLD;
  movimento = movX || movY || movZ;
}

// ===================================================
//                       Setup
// ===================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== CardioIA – Parte 2 (MQTT) – Inicializando ===");

  // IOs
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT); // HIGH = online

  // Sensores
  dht.begin();
  Wire.begin(21, 22);
  mpu.begin();
  mpu.calcGyroOffsets(true);  // calibra o giroscópio 

  // Redes
  conectaWiFi();
  conectaMQTT();

  // Janelas iniciais
  inicioJanelaBPM = millis();
  lastSample      = millis();
  lastPublish     = millis();
}

// ===================================================
//                        Loop
// ===================================================
void loop() {
  // Mantenho MQTT vivo e reconecto se cair
  if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) {
    conectaMQTT();
  }
  mqtt.loop();

  // Atualizações contínuas
  updateBPM();
  updateMovimento();

  unsigned long now = millis();

  // Leitura do DHT a cada 1s 
  static float t = NAN, h = NAN;
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;

    h = dht.readHumidity();
    t = dht.readTemperature();

    // fallback simples quando o DHT retorna NaN
    if (isnan(t) || isnan(h)) {
      t = 26.0;
      h = 45.0;
      Serial.println("[DHT] leitura inválida, usando defaults 26/45");
    }
  }

  // Publico a cada 3s no formato curto esperado pelo Node-RED
  if (now - lastPublish >= PUBLISH_MS) {
    lastPublish = now;

    bool online = digitalRead(SWITCH_PIN) == HIGH;
    bool alerta = (bpm > MAX_BPM) || (t > MAX_TEMP);

    // LED de alerta para feedback visual
    digitalWrite(LED_PIN, alerta ? HIGH : LOW);

    // *** JSON compacto no estilo do debug do Node-RED ***
    // Ex.: { t: 39.6, h: 48.3, b: 120, o: true, a: false }
    // Obs.: aqui eu monto “na mão” para manter os nomes curtinhos.
    char payload[160];
    // online/alerta como true/false (sem aspas)
    const char* oStr = online ? "true" : "false";
    const char* aStr = alerta ? "true" : "false";
    // formatação com 1 casa para manter próximo ao print do Node-RED
    snprintf(payload, sizeof(payload),
             "{ t: %.1f, h: %.1f, b: %d, o: %s, a: %s }",
             t, h, bpm, oStr, aStr);

    Serial.print("Publicando: ");
    Serial.println(payload);

    if (mqtt.connected()) {
      mqtt.publish(MQTT_TOPIC, payload);
    }
  }
}
