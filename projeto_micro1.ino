//Esse é o código que implementa um jogo lúdico de perguntas e respostas para crianças de 8 a 10 anos.
//O jogo inicia quando uma lanterna é aproximada do LDR e assim a primeira pergunta aparece no LCD. 
//Com dois botões, é possível mudar as alternativas com um dos botões e confirmar uma delas com o outro.
//Se a resposta estiver correta, um LED verde se acende, senão um vermelha é aceso. Ao final, o buzzer é tocado, 
//a luz amarela varia em PWM e a pontuação é mostrada no LCD.

//Importa a biblioteca do LCD e define os pinos conectados ao display.

#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 4, 3, 2, 10);


//Declara os pinos usados para os sensores, LEDs, botões e buzzer

const int ldrPin = A0;
const int ledVerde = 5;
const int ledAmarelo = 6;
const int ledVermelho = 7;
const int buzzer = 13;
const int botaoP = 8;
const int botaoR = 9;


//Desenhos de emojis personalizados para o LCD (feliz, neutra, triste)


byte charFeliz[8] = { B00000, B01010, B01010, B00000, B00000, B10001, B01110, B00000 };
byte charNeutra[8] = { B00000, B01010, B01010, B00000, B00000, B01110, B00000, B00000 };
byte charTriste[8] = { B00000, B01010, B01010, B00000, B00000, B01110, B10001, B00000 };


//Declaração do total de perguntas e dos Bancos de dados do jogo: perguntas, alternativas e índice da alternativa correta para cada
//pergunta.


const int TOTAL_PERGUNTAS = 10;
String perguntas[TOTAL_PERGUNTAS] = {
 "2+2?",
 "Capital do Brasil?",
 "Cor do sol?",
 "Planeta vermelho?",
 "Maior oceano?",
 "5x3 = ?",
 "Mamifero?",
 "Continente Egito?",
 "Dias no ano?",
 "Cor da grama?"
};
String alternativas[TOTAL_PERGUNTAS][4] = {
 {"1) 3", "2) 4", "3) 5", "4) 6"},
 {"1) SP", "2) RJ", "3) Brasilia", "4) Curitiba"},
 {"1) Azul", "2) Amarelo", "3) Verde", "4) Vermelho"},
 {"1) Terra", "2) Jupiter", "3) Marte", "4) Saturno"},
 {"1) Atlantico", "2) Pacifico", "3) Indico", "4) Artico"},
 {"1) 15", "2) 12", "3) 20", "4) 8"},
 {"1) Peixe", "2) Cobra", "3) Gato", "4) Galinha"},
 {"1) Europa", "2) Asia", "3) Africa", "4) America"},
 {"1) 360", "2) 365", "3) 370", "4) 366"},
 {"1) Azul", "2) Rosa", "3) Verde", "4) Cinza"}
};
int respostaCorreta[TOTAL_PERGUNTAS] = {1, 2, 1, 2, 1, 0, 2, 2, 1, 2};




//Variáveis de controle de estado do jogo, pontuação, pergunta atual, //alternativa selecionada e debounce dos botões.


int leituraLDR;
int perguntaAtual = 0;
int etapaAlternativa = 0;
int pontuacao = 0;
bool jogoAtivo = false;
bool estadoBotaoP = false;
bool estadoBotaoR = false;


//Inicializa o LCD e os pinos (com suas respectivas funções, INPUT ou
//OUTPUT), mostra a tela inicial aguardando a ativação pelo LDR, define
//os emojis personalizados.


void setup() {
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(botaoP, INPUT);
  pinMode(botaoR, INPUT);
  pinMode(ldrPin, INPUT);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("Aproxime a");
  lcd.setCursor(0, 1);
  lcd.print("lanterna...");
  lcd.createChar(0, charFeliz);
  lcd.createChar(1, charNeutra);
  lcd.createChar(2, charTriste);
}


void loop() {
  leituraLDR = analogRead(ldrPin); //Lê o valor do LDR
  //Serial.println(leituraLDR);
 
  //Quando o valor do LDR indica a presença da mão, o jogo começa.
  if(!jogoAtivo && leituraLDR > 300) {
    delay(300);
    jogoAtivo = true;
    lcd.clear();
    lcd.print("Jogo Iniciado!"); //É exibida a mensagem de “Jogo Iniciado!”
    etapaAlternativa = 0;
    perguntaAtual = 0;
    delay(3000);
    //Aguarda os botões voltarem para HIGH (repouso) caso eles tenham ficado apertados por engano
    while (digitalRead(botaoP) == HIGH || digitalRead(botaoR) == HIGH) {
      delay(10); // aguarda em loop até soltar
      Serial.println("erro");
    }
    mostrarPergunta(); //Jogo começa, chamando a função que vai mostrar a primeira pergunta
 }


  //Controla a rolagem das alternativas. etapaAlternativa varia de 1 a 4 (evita 0).
  //A linha do LCD é atualizada com a nova alternativa.
  if (estadoBotaoP && digitalRead(botaoP) == HIGH) {
    estadoBotaoP = false;
    etapaAlternativa++;
    if (etapaAlternativa > 4) etapaAlternativa = 1;
    mostrarAlternativa();
    delay(200);
  }
  if (digitalRead(botaoP) == LOW) {
    estadoBotaoP = true;
  }


  //Quando o botão de confirmação é pressionado, o código verifica se a alternativa selecionada é a correta.
  //Dependendo disso, acende o LED correspondente, incrementa a pontuação e avança para a próxima pergunta.
  if (estadoBotaoR && digitalRead(botaoR) == HIGH) {
    estadoBotaoR = false;
    if (etapaAlternativa > 0) {
      if (etapaAlternativa - 1 == respostaCorreta[perguntaAtual]) {
        pontuacao++;
        digitalWrite(ledVerde, HIGH);
      }else {
        digitalWrite(ledVermelho, HIGH);
      }
      delay(1000);
      apagarLEDs();
      perguntaAtual++;
      etapaAlternativa = 0;


      if (perguntaAtual >= TOTAL_PERGUNTAS) {
        fimDoJogo();
      } else {
          mostrarPergunta();
      }
    }
    delay(200);
  }


  if (digitalRead(botaoR) == LOW) {
    estadoBotaoR = true;
  }
}


//Exibe a pergunta e inicializa a exibição da primeira alternativa.
void mostrarPergunta() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(perguntas[perguntaAtual].substring(0, 16)); // Linha 0: pergunta
  etapaAlternativa = 1;
  mostrarAlternativa();
}


//Mostra uma alternativa na linha de baixo do LCD.
void mostrarAlternativa() {
  lcd.setCursor(0, 1);           // Linha 1
  lcd.print("                "); // Limpa linha
  lcd.setCursor(0, 1);
  lcd.print(alternativas[perguntaAtual][etapaAlternativa - 1].substring(0, 16));
}


//Mostra a pontuação final com emoji correspondente e LED amarelo via PWM. Reinicia o jogo após 5 segundos.
void fimDoJogo() {
 jogoAtivo = false;
 lcd.clear();
 lcd.print("Pontuacao:");
 lcd.print(pontuacao);
 lcd.setCursor(0, 1);
 tone(buzzer, 1000, 300);
 for(int i = 0; i<=pontuacao; i++){
  for(int j =0; j<=255; j+=5){
    analogWrite(ledAmarelo, j);
    delay(30);
    Serial.println(j);
  }
  for(int j = 255; j>=0; j-=5){
    analogWrite(ledAmarelo, j);
    delay(30);
    Serial.println(j);
  }
 }
 //analogWrite(ledAmarelo, pontuacao * 25);
 lcd.setCursor(0, 2);
 if(pontuacao < 4){
  lcd.write(byte(2)); // 😢 triste
  lcd.print("Tente outra vez!");
 } else if(pontuacao < 7){
  lcd.write(byte(1)); // 😐 neutra
  lcd.print(" Quase la!");
 } else {
  lcd.write(byte(0)); // 😊 feliz
  lcd.print(" Parabens!");
 }
 delay(5000);
 tone(buzzer, 1000, 300);
 lcd.clear();
 //lcd.print("Aproxime a mao...");
 lcd.print("Aproxime a");
 lcd.setCursor(0, 1);
 lcd.print("lanterna...");
 perguntaAtual = 0;
 etapaAlternativa = 0;
 pontuacao = 0;
 apagarLEDs();
}


//Desliga todos os LEDs.
void apagarLEDs() {
 digitalWrite(ledVerde, LOW);
 digitalWrite(ledVermelho, LOW);
 digitalWrite(ledAmarelo, LOW);
}
