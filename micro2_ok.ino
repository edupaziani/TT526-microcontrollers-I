#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// #define SS_PIN 45
// #define RST_PIN 7
// #define SCK_PIN 39
// #define MOSI_PIN 41
// #define MISO_PIN 40
#define SS_PIN 5
#define RST_PIN 4
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19

//Objetos
MFRC522 rfid(SS_PIN, RST_PIN);

// Configuração do Wi-Fi
const char* ssid = "S25+";
const char* password = "5555555555";

// Token e Chat ID do bot do Telegram
const String botToken = "8432971243:AAGsey3M4agy20nUH3_gUvd6iA9eG9AEQOs";
const String chatId = "1029641195";

// Estrutura de dados para cada usuário
struct Usuario {
  String id;
  String nome;
  bool presente;
  unsigned long entrada;
  unsigned long tempoPermanencia;
  unsigned long tempoTotal;
};

// Variáveis globais
const int MAX_USUARIOS = 20;
Usuario usuarios[MAX_USUARIOS];
int totalUsuarios = 0;
bool relatorioMostrado = false;

// Variáveis para controle do relatório
  bool mostrandoRelatorio = false;
  unsigned long tempoInicioRelatorio = 0;
  const unsigned long TEMPO_RELATORIO_MS = 10000; // 10 segundos
  bool relatorioMostrado = false; // Para controlar se o relatório já foi mostrado

void setup() {
  Serial.begin(115200);

  // Inicializar SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

  // Inicializar RFID
  // mfrc522.PCD_Init();
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());
  Serial.println();
  SerialBT.println("Sistema de Controle de Acesso Iniciado");
  SerialBT.println("Aguardando comandos...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(500);
    return;
  }

  int indice = encontrarIndicePorUID(rfid.uid.uidByte, rfid.uid.size);

  if (indice >= 0) {
    registrarMovimento(indice);
    enviarTelegram("🔄 Movimento registrado para " + usuarios[indice].nome);
  } else {
    String uidStr = gerarUIDString(rfid.uid.uidByte, rfid.uid.size);
    cadastrarNovoUsuario(uidStr, "Usuario_" + String(totalUsuarios + 1));
    enviarTelegram("🆕 Novo usuario cadastrado: " + uidStr);
  }

  rfid.PICC_HaltA();
  delay(2000);
}

String gerarUIDString(byte *uid, byte tamanho) {
  String uidStr = "";
  for (byte i = 0; i < tamanho; i++) {
    uidStr += String(uid[i], HEX);
    if (i < tamanho - 1) uidStr += ":";
  }
  return uidStr;
}

int encontrarIndicePorUID(byte *uid, byte tamanho) {
  String uidStr = gerarUIDString(uid, tamanho);
  for (int i = 0; i < totalUsuarios; i++) {
    if (usuarios[i].id == uidStr) return i;
  }
  return -1;
}

void cadastrarNovoUsuario(String id, String nome) {
  if (totalUsuarios < MAX_USUARIOS) {
    usuarios[totalUsuarios].id = id;
    usuarios[totalUsuarios].nome = nome;
    usuarios[totalUsuarios].presente = true;
    usuarios[totalUsuarios].entrada = millis();
    usuarios[totalUsuarios].tempoPermanencia = 0;
    usuarios[totalUsuarios].tempoTotal = 0;
    Serial.println("Usuario cadastrado com sucesso!");
    totalUsuarios++;
    relatorioMostrado = false;
  } else {
    Serial.println("Erro: Limite maximo de usuarios atingido!");
  }
}

void registrarMovimento(int indice) {
  if (usuarios[indice].presente) {
    unsigned long tempoEstaPermanencia = millis() - usuarios[indice].entrada;
    usuarios[indice].tempoTotal += tempoEstaPermanencia;
    usuarios[indice].tempoPermanencia = 0;
    usuarios[indice].presente = false;
    Serial.println("SAIDA: " + usuarios[indice].nome);
  } else {
    usuarios[indice].presente = true;
    usuarios[indice].entrada = millis();
    Serial.println("ENTRADA: " + usuarios[indice].nome);
    relatorioMostrado = false;
  }
}

void enviarTelegram(String mensagem) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatId +
                 "&text=" + urlencode(mensagem);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.print("Notificacao enviada. Codigo: ");
      Serial.println(httpCode);
    } else {
      Serial.print("Erro ao enviar: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi nao conectado!");
  }
}

String urlencode(String str) {
  String encoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      char hex1 = (c >> 4) & 0xF;
      char hex2 = c & 0xF;
      encoded += char(hex1 > 9 ? hex1 - 10 + 'A' : hex1 + '0');
      encoded += char(hex2 > 9 ? hex2 - 10 + 'A' : hex2 + '0');
    }
  }
  return encoded;
}
