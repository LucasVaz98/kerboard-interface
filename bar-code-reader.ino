/*
   TO STOP PRINTING DEBUG MESSAGES CHANGE DEBUG MACRO ON usb.h to 0 or FALSE
   ALSO IN settings.h
*/
#include "usbhub.h"
#include "hiduniversal.h"
#include "hidboot.h"
#include "Usb.h"
#include <SoftwareSerial.h>

#define TXD 6
#define RXD 7
#define BAUD_RATE 115200
#define PIN_TO_GET_PROBLEM 5
#define TIME_IN_DELAY_TO_SIMULATE 30000

byte QtdProblemas = 0;
byte Trasmitindo = 0;

SoftwareSerial ESPSerial(RXD, TXD);// RX, TX

class MyParser : public HIDReportParser {
  public:
    MyParser();
    void Parse(USBHID *hid, bool is_rpt_mid, uint8_t len, uint8_t *buf);
  protected:
    uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
    virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
    virtual void OnScanFinished();
};

MyParser::MyParser() {}

void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf, buf[i]);
    }

    return;
  }
}

uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}

void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);
  if (DEBUG_BARCODE)
  {
    Serial.print(ascii);
    Serial.print("(");
    Serial.print(char(ascii));
    Serial.print(") - ");
  }

  ESPSerial.write(ascii);
  Trasmitindo = 1;
}

void MyParser::OnScanFinished()
{
  if (DEBUG_BARCODE)
    Serial.println(" - Finished");

  ESPSerial.write(0x19);
  Trasmitindo = 0;
}

USB          Usb;
USBHub       Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser     Parser;

void (*resetFunc) (void) = 0;

void setup() {

  ESPSerial.begin(BAUD_RATE);
  pinMode(PIN_TO_GET_PROBLEM, INPUT_PULLUP);

  pinMode(PIN_TO_RESET, OUTPUT);
  digitalWrite(PIN_TO_RESET, !STATE_IN_RESET);
  Serial.begin(BAUD_RATE);
  if (DEBUG_BARCODE)
  {
    Serial.begin(BAUD_RATE);
    Serial.println("BarCodeReader.ino -> setup() ->Start");
  }

  delay(200);

  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
    delay(15000);
  }
  delay(200);
  Hid.SetReportParser(0, &Parser);
}

void loop() {
  if (digitalRead(PIN_TO_GET_PROBLEM) == 0) // PARA PODERMOS SIMULAR O LEITOR TRAVADO
    delay(TIME_IN_DELAY_TO_SIMULATE);

  QtdProblemas += Usb.Task();

  if (QtdProblemas >= 200) { // em caso de overflow, deixa a vairável travada em 199
    QtdProblemas = 199;
  }

  if (ESPSerial.available() > 0) {
    char a = ESPSerial.read();
    if (Trasmitindo == 0) {
      ESPSerial.write(0x07);
      ESPSerial.write(QtdProblemas);
    }
    //Serial.println();
    QtdProblemas = 0;
  }
}
