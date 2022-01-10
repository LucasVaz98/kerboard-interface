/*
   This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
*/
#include <BleKeyboard.h>

#define BAUD_RATE 115200
#define DEBUG 0
#define TIME_BTWN_CODES 1000
#define LED_OUTPUT 2
#define NAME "BLE Keyboard"
#define TIME_BEFORE_RESET 2000
#define PIN_RESET 4
#define TIME_IN_RESET 10000
#define TIME_AFTER_RESET 400
#define TIME_BETWEEN_ESP_RESET 60*1000
#define TIME_BETWEEN_ARD_RESET 2*60*60*1000


BleKeyboard bleKeyboard(NAME);

int flag = 1;
int Recebendo = 0;
int countKeepAlive = 0;
unsigned int timeLastChar = 0;
uint16_t *code;
uint8_t inicio = 0x00;
unsigned long int cont = 0;
int missedKeepAlives = 0;
unsigned long int t_last_reset_ard = 0;
unsigned int t_last_reset_esp = 0;


void setup()
{
  pinMode(LED_OUTPUT, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);

  digitalWrite(LED_OUTPUT, HIGH);
  digitalWrite(PIN_RESET, HIGH);

  esp32TurnOn();

  Serial.begin(BAUD_RATE);
  Serial2.begin(BAUD_RATE);

  if (DEBUG)
    Serial.println("Starting BLE work!");

  flag = 1;
  beginBT();

  delay(500);
}

void loop()
{
  char HB = 0x0;
  char LB = 0x0;
  int erros_leitor = 0;

  //delay para dar tempo de monitorar o keep alive
  delay(5);

  countKeepAlive++;

  if ((countKeepAlive > 500) && (Recebendo == 0)) {

    Serial2.write('a');
    delay(100);

    inicio = 0x00;
    if (Serial2.available() > 0) {
      inicio = Serial2.read();
    } else missedKeepAlives ++;

    if (inicio == 0x07) {

      if (Serial2.available() > 0) {
        LB = Serial2.read();
        //if (Serial2.available() > 0) {
        //  HB = Serial2.read();
        //s}

        erros_leitor = ((HB << 8) + LB);

        if (erros_leitor > 100) {
          missedKeepAlives++;
        } else {
          missedKeepAlives = 0;
        }

      } else missedKeepAlives++;

    } else {
      //se não era caracter do keepalive, manda para bluetooth
      if ((inicio != 0x00)) { //se estou recebendo alguma coisa, tenho que imprimir
        Serial.print("eh o caso: ");
        Serial.write(inicio);
      }
    }

    if (DEBUG == 1) {
      Serial.println(erros_leitor);
      Serial.println(missedKeepAlives);
    }
    if (missedKeepAlives > 4) {
      resetArduino();
      missedKeepAlives = 0;
    }
    countKeepAlive = 0;
  }

  if (millis() - t_last_reset_ard > TIME_BETWEEN_ARD_RESET)
  {
    resetArduino();
    t_last_reset_ard = millis();
  }

  if (!bleKeyboard.isConnected() &&  millis() - t_last_reset_esp > TIME_BETWEEN_ESP_RESET)
  {
    bleKeyboard.begin();
    beginBT();
    ESP.restart();
    return;
  } else if (bleKeyboard.isConnected())
  {
#if (DEBUG)
    flag = test_connection(flag);
#endif

    while (Serial2.available() > 0)
    {
      receive_code_from_arduino();
    }
  }
}

int test_connection(int should_test)
{
  if (should_test)
  {
    Serial.println("Sending 'Hello world'...");
    bleKeyboard.print("Hello world");
    delay(1000);
    should_test = 0;
  }
  return should_test;
}

void receive_code_from_arduino()
{
  int x = 0;
  if (Serial2.available()) {
    Recebendo = 1;
    while (Serial2.available())
    {
      code[x++] = Serial2.read();
    }
  }
#if(DEBUG)
  Serial.println(code);
#endif

  for (int i = 0; i < x; i++)
  {
    send_string(i);
  }
}

void send_string(int x)
{
  if (code[x] == 0x19)
  {
    bleKeyboard.write(0x0D);
    bleKeyboard.write(0x0A);
    Recebendo = 0;
  }
  else {
    if ((inicio != 0x00) && (inicio != 0x07)) {
      bleKeyboard.write(inicio);
      inicio = 0x00;
    }
    bleKeyboard.write(code[x]);
  }

  if (DEBUG)
  {
    Serial.print((char)code[x]);
  }
}

void resetArduino()
{
  char c;

  digitalWrite(LED_OUTPUT, LOW);
  digitalWrite(PIN_RESET, LOW);
  delay(TIME_IN_RESET);
  digitalWrite(LED_OUTPUT, HIGH);
  digitalWrite(PIN_RESET, HIGH);
  delay(TIME_AFTER_RESET);

  while (Serial2.available() > 0)  {
    c = Serial2.read(); //limpa o buffer depois de resetar
  }
  Recebendo = 0;
}

void esp32TurnOn()
{
  for (int j = 0; j < 3; j++)
  {
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(LED_OUTPUT, LOW);
      delay(333);
      digitalWrite(LED_OUTPUT, HIGH);
      delay(333);
    }
    delay(500);
  }
}

void beginBT()
{
  bleKeyboard.begin();
  for (int i = 0; i < 1.; i++)
  {
    digitalWrite(LED_OUTPUT, LOW);
    delay(333);
    digitalWrite(LED_OUTPUT, HIGH);
    delay(666);
  }
  delay(500);
}
