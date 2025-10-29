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
MFRC522 mfrc522(SS_PIN, RST_PIN);

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
bool aguardandoNome = false;
String idAguardandoNome = "";
String nomeRecebido = "";

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
  mfrc522.PCD_Init();

  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("========================================");
  Serial.println("Sistema de Controle de Acesso Iniciado");
  Serial.println("========================================\n");
  Serial.println("Aproxime um cartão para começar o cadastro, caso ainda não tenha, ou para ser liberado no sistema caso contrário.");
  Serial.println("Se estiver com dúvidas, digite AJUDA para ver a lista de comandos.");
  Serial.println("Aguardando comandos...");
}

void loop() {

  // Verificar se há dados na Serial
  if (Serial.available()) {
    String mensagem = Serial.readString();
    mensagem.trim();
    
    if (aguardandoNome) {
      nomeRecebido = mensagem;
      cadastrarNovoUsuario(idAguardandoNome, nomeRecebido);
      aguardandoNome = false;
      idAguardandoNome = "";
      nomeRecebido = "";
    } else {
      processarComandoBluetooth(mensagem);
    }
  }

// Verificar se há cartão RFID presente
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String id = lerIDRFID();
    
    // Verificar se é um usuário conhecido
    int indice = buscarUsuarioPorID(id);
    
    if (indice == -1) {
      // Novo usuário - solicitar cadastro
      Serial.println("Novo usuario detectado! ID: " + id);
      Serial.println("Por favor, digite o nome para cadastro:");
      idAguardandoNome = id;
      aguardandoNome = true;
      
      mostrarTelaNovoUsuario(id);
      enviarTelegram("🆕 Novo cartão detectado! ID: " + id + "\nAguardando cadastro...");
    } else {
      // Usuário existente - registrar entrada/saída
      registrarMovimento(indice);
      enviarTelegram("🔄 Movimento registrado para: " + usuarios[indice].nome);
    }
    
    mfrc522.PICC_HaltA();
    delay(1000);
  }
   // Verificar se todos saíram E se há usuários cadastrados E se o relatório ainda não foi mostrado
  if (todosSairam() && totalUsuarios > 0 && !mostrandoRelatorio && !relatorioMostrado) {
    mostrarRelatorioOLED();
    enviarRelatorioTelegram(); // Envia relatório via Telegram
    mostrandoRelatorio = true;
    relatorioMostrado = true; // Marca que o relatório já foi mostrado
    tempoInicioRelatorio = millis();
  }
  
  // Verificar se já passou o tempo do relatório
  if (mostrandoRelatorio && (millis() - tempoInicioRelatorio > TEMPO_RELATORIO_MS)) {
    mostrandoRelatorio = false;
    mostrarTelaNormal();
  }
  
  // Resetar a flag relatorioMostrado quando alguém entrar
  if (!todosSairam() && relatorioMostrado) {
    relatorioMostrado = false;
  }
  
  delay(100);
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

// Função para enviar relatório completo via Telegram
void enviarRelatorioTelegram() {
  String mensagem = "📊 *RELATÓRIO FINAL* 📊\n";
  mensagem += "Tempo Total Acumulado\n";
  mensagem += "----------------------\n";
  
  for (int i = 0; i < totalUsuarios; i++) {
    mensagem += "👤 " + usuarios[i].nome + ": " + formatarTempo(usuarios[i].tempoTotal) + "\n";
  }
  
  mensagem += "\nTodos os usuários saíram do local.";
  enviarTelegram(mensagem);
}

void mostrarTelaNormal() {
  Serial.println("");
  Serial.println("Sistema Ativo");
  Serial.println("Aproxime o cartao");
  Serial.println("Usuarios: " + String(totalUsuarios));
  Serial.println("");
}

void mostrarTelaNovoUsuario(String id) {
  Serial.println("");
  Serial.println("NOVO USUARIO");
  Serial.println("ID: " + id);
  Serial.println("Cadastre via serial");
  Serial.println("");
}

String lerIDRFID() {
  String id = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    id.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    id.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  id.toUpperCase();
  return id;
}

int buscarUsuarioPorID(String id) {
  for (int i = 0; i < totalUsuarios; i++) {
    if (usuarios[i].id == id) {
      return i;
    }
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
    Serial.println("Nome: " + nome);
    Serial.println("ID: " + id);
    
    mostrarTelaCadastrado(nome);
    totalUsuarios++;
    mostrarStatusBluetooth();

    // Enviar notificação de novo usuário para Telegram
    enviarTelegram("✅ *NOVO USUÁRIO CADASTRADO!*\n👤 Nome: " + nome + "\n🆔 ID: " + id);
    
    delay(2000);
    mostrarTelaNormal();
    
    // Resetar flag do relatório quando novo usuário entrar
    relatorioMostrado = false;
  } else {
    Serial.println("Erro: Limite maximo de usuarios atingido!");
    enviarTelegram("❌ Erro: Limite máximo de usuários atingido!");
  }
}

void mostrarTelaCadastrado(String nome) {
  Serial.println("");
  Serial.println("CADASTRADO!");
  Serial.println("Nome: " + nome);
  Serial.println("Bem-vindo!");
  Serial.println("");
}

void registrarMovimento(int indice) {
  if (usuarios[indice].presente) {
    // Registrar saída - calcular tempo desta permanência
    unsigned long tempoEstaPermanencia = millis() - usuarios[indice].entrada;
    
    // Adicionar ao tempo total acumulado
    usuarios[indice].tempoTotal += tempoEstaPermanencia;
    
    // Resetar tempoPermanencia para a próxima entrada
    usuarios[indice].tempoPermanencia = 0;
    usuarios[indice].presente = false;
    
    Serial.println("SAIDA: " + usuarios[indice].nome);
    Serial.println("Tempo desta permanencia: " + formatarTempo(tempoEstaPermanencia));
    Serial.println("Tempo total acumulado: " + formatarTempo(usuarios[indice].tempoTotal));
  
    mostrarTelaSaida(usuarios[indice].nome, tempoEstaPermanencia, usuarios[indice].tempoTotal);

     // Enviar notificação de saída para Telegram
    enviarTelegram("🚪 *SAÍDA REGISTRADA*\n👤 " + usuarios[indice].nome + 
                   "\n⏱️ Tempo desta permanência: " + formatarTempo(tempoEstaPermanencia) +
                   "\n📊 Tempo total acumulado: " + formatarTempo(usuarios[indice].tempoTotal));
  } else {
    // Registrar entrada
    usuarios[indice].presente = true;
    usuarios[indice].entrada = millis();
    
    Serial.println("ENTRADA: " + usuarios[indice].nome);
    Serial.println("Tempo total acumulado: " + formatarTempo(usuarios[indice].tempoTotal));
    
    mostrarTelaEntrada(usuarios[indice].nome, usuarios[indice].tempoTotal);
    
    // Enviar notificação de entrada para Telegram
    enviarTelegram("🚪 *ENTRADA REGISTRADA*\n👤 " + usuarios[indice].nome + 
                   "\n📊 Tempo total acumulado: " + formatarTempo(usuarios[indice].tempoTotal));


    // Resetar flag do relatório quando alguém entrar
    relatorioMostrado = false;
  }

  delay(2000);
  
  // Só mostra status se não estiver mostrando relatório
  if (!mostrandoRelatorio) {
    mostrarStatusBluetooth();
    mostrarTelaNormal();
  }
}

void mostrarTelaSaida(String nome, unsigned long tempoEstaPermanencia, unsigned long tempoTotal) {
  Serial.println("");
  Serial.println("SAIDA REGISTRADA");
  Serial.println("Nome: " + nome);
  Serial.println("Esta: " + formatarTempo(tempoEstaPermanencia));
  Serial.println("Total: " + formatarTempo(tempoTotal));
  Serial.println("");
}

void mostrarTelaEntrada(String nome, unsigned long tempoTotal) {
  Serial.println("");
  Serial.println("ENTRADA");
  Serial.println("Bem-vindo!");
  Serial.println(nome);
  Serial.println("Total: " + formatarTempo(tempoTotal));
  Serial.println("");
}

bool todosSairam() {
  for (int i = 0; i < totalUsuarios; i++) {
    if (usuarios[i].presente) {
      return false;
    }
  }
  return true;
}

void mostrarRelatorioOLED() {
  Serial.println("");
  //display.setFont(ArialMT_Plain_10);
  //display.setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.println("RELATORIO FINAL");
  Serial.println("TOTAL ACUMULADO");
  Serial.println("----------------");
  
  int linha = 36;
  for (int i = 0; i < totalUsuarios; i++) {
    String linhaTexto = usuarios[i].nome.substring(0, 8) + ":";
    linhaTexto += formatarTempo(usuarios[i].tempoTotal);
    Serial.println(linhaTexto);
    //display.drawString(0, linha, linhaTexto);
    linha += 10;
  }
  
  Serial.println("");
  
  // Também enviar relatório via Bluetooth
  // Serial.println("\n--- RELATORIO FINAL ---");
  // Serial.println("TEMPO TOTAL ACUMULADO");
  // for (int i = 0; i < totalUsuarios; i++) {
  //     String tempoTotal = formatarTempo(usuarios[i].tempoTotal);
  //     Serial.println(usuarios[i].nome + ": " + tempoTotal);
  //   }
  //   Serial.println("---------------------\n");
} 

void mostrarStatusBluetooth() {
  Serial.println("\n--- STATUS ATUAL ---");
  int presentes = 0;
  for (int i = 0; i < totalUsuarios; i++) {
    String status = usuarios[i].presente ? "PRESENTE" : "AUSENTE";
    String tempoInfo = usuarios[i].presente ? 
      " (Tempo atual: " + formatarTempo(millis() - usuarios[i].entrada) + ")" :
      " (Total: " + formatarTempo(usuarios[i].tempoTotal) + ")";
    
    Serial.println(usuarios[i].nome + ": " + status + tempoInfo);
    if (usuarios[i].presente) presentes++;
  }
  
  Serial.println("Total presentes: " + String(presentes));
  Serial.println("---------------------\n");
}

String formatarTempo(unsigned long milliseconds) {
  unsigned long segundos = milliseconds / 1000;
  unsigned long minutos = segundos / 60;
  unsigned long horas = minutos / 60;
  
  segundos = segundos % 60;
  minutos = minutos % 60;
  
  char buffer[20];
  if (horas > 0) {
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", horas, minutos, segundos);
  } else {
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", minutos, segundos);
  }
  
  return String(buffer);
}

void processarComandoBluetooth(String comando) {
  comando.toUpperCase();
  
  if (comando == "STATUS") {
    mostrarStatusBluetooth();
  } else if (comando == "LISTA") {
    Serial.println("\n=== LISTA DE USUARIOS ===");
    for (int i = 0; i < totalUsuarios; i++) {
      Serial.println(String(i+1) + ". " + usuarios[i].nome + 
                      " (ID: " + usuarios[i].id + 
                      ", Total: " + formatarTempo(usuarios[i].tempoTotal) + ")");
    }
    Serial.println("---------------------\n");
  } else if (comando == "RELATORIO") {
    Serial.println("\n--- RELATORIO DETALHADO ---");
    for (int i = 0; i < totalUsuarios; i++) {
      String tempoTotal = formatarTempo(usuarios[i].tempoTotal);
      Serial.println(usuarios[i].nome + ": " + tempoTotal);
    }
    Serial.println("---------------------\n");
  } else if (comando == "AJUDA" || comando == "HELP") {
    Serial.println("\n--- COMANDOS DISPONIVEIS ---");
    Serial.println("STATUS - Mostrar status atual");
    Serial.println("LISTA - Listar todos usuarios");
    Serial.println("RELATORIO - Mostrar tempos totais");
    Serial.println("AJUDA - Mostrar esta ajuda");
    Serial.println("---------------------\n");
  } else {
    Serial.println("Comando nao reconhecido. Digite AJUDA para ver os comandos.");
  }
}
