#include <WiFiS3.h>
#include <DHT.h>

// --- Configurações de Hardware ---
#define DHTPIN 1 
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

// --- Ajustes de Calibração (Offsets) ---
// Se o sensor marcar 2 graus a mais que o real, coloque -2.0
const float CALIB_TEMP = 0.0; 
const float CALIB_UMID = 0.0;

// --- Configurações de Rede ---
char ssid[] = "";
char pass[] = "";
char server[] = "";
int port = ;

WiFiClient client;

// --- Controle de Tempo e Amostragem ---
unsigned long previousMillis = 0;
const long interval = 600000; // 10 minutos para teste
const int numLeituras = 3;    // Número de amostras para a média

// Protocolo CO2 MH-Z19
byte readCO2[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
unsigned char response[9];

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600); // Sensor CO2
  dht.begin();
  
  Serial.println("\n--- SISTEMA MARE DE DADOS ---");
  Serial.println("\n--- Por: João Euzébio ---");
  conectarWiFi();
}

// --- Tratamento de Dados: Média Móvel ---
float lerTemperaturaMedia() {
  float soma = 0;
  int contagem = 0;
  for (int i = 0; i < numLeituras; i++) {
    float t = dht.readTemperature();
    if (!isnan(t)) {
      soma += t;
      contagem++;
    }
    delay(2500); // Pequeno intervalo entre amostras do DHT22
  }
  return (contagem > 0) ? (soma / contagem) + CALIB_TEMP : NAN;
}

float lerUmidadeMedia() {
  float soma = 0;
  int contagem = 0;
  for (int i = 0; i < numLeituras; i++) {
    float h = dht.readHumidity();
    if (!isnan(h)) {
      soma += h;
      contagem++;
    }
    delay(500);
  }
  return (contagem > 0) ? (soma / contagem) + CALIB_UMID : NAN;
}

// --- Leitura e Checksum do CO2 ---
int getCO2() {
  while(Serial1.available()) Serial1.read(); // Limpa buffer
  Serial1.write(readCO2, 9);
  memset(response, 0, 9);
  
  if (Serial1.readBytes(response, 9) == 9) {
    byte check = 0;
    for (int i = 1; i < 8; i++) check += response[i];
    check = 0xFF - check;
    check += 1;

    if (response[0] == 0xFF && response[1] == 0x86 && response[8] == check) {
      return (256 * (int)response[2]) + (int)response[3];
    }
  }
  return -1; // Falha na leitura ou Checksum
}

void conectarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("[WIFI] Conectando...");
    WiFi.begin(ssid, pass);
    int t = 0;
    while (WiFi.status() != WL_CONNECTED && t < 20) {
      delay(500); Serial.print("."); t++;
    }
    if (WiFi.status() == WL_CONNECTED) Serial.println(" OK!");
    else Serial.println(" FALHA.");
  }
}

void enviarDados(float t, float h, int co2) {
  conectarWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connect(server, port)) {
      String data = "{\"sensor_id\":\"arduino_01\",\"temp\":" + String(t) + 
                    ",\"umid\":" + String(h) + ",\"co2\":" + String(co2) + "}";
      
      client.println("POST /api/dados HTTP/1.1");
      client.print("Host: "); client.println(server);
      client.println("Content-Type: application/json");
      client.print("Content-Length: "); client.println(data.length());
      client.println("Connection: close");
      client.println();
      client.print(data);
      client.stop();
      Serial.println("[SERVER] Dados enviados com sucesso.");
    } else {
      Serial.println("[ERRO] Servidor offline.");
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;

    Serial.println("\n--- Iniciando Amostragem ---");
    
    // 1. Tratamento de dados (Média e Calibração)
    float t = lerTemperaturaMedia();
    float h = lerUmidadeMedia();
    int co2 = getCO2();

    // 2. Correção de Erros (Validação)
    if (isnan(t) || isnan(h)) {
      Serial.println("[ALERTA] Erro crítico no DHT22. Pulando ciclo.");
    } else if (co2 < 0) {
      Serial.println("[ALERTA] Erro no CO2 (Checksum/Conexão).");
    } else {
      // 3. Log Local
      Serial.print("Temp: "); Serial.print(t);
      Serial.print(" | Umid: "); Serial.print(h);
      Serial.print(" | CO2: "); Serial.println(co2);

      // 4. Envio
      enviarDados(t, h, co2);
    }
  }
}