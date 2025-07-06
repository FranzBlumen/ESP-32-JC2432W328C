#include <WiFi.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include "CST820.h"

// WiFi / MQTT
const char* ssid = "franzblumen";
const char* password = "30280775";
const char* mqtt_server = "192.168.1.106";

// TFT & Touch
TFT_eSPI tft = TFT_eSPI();
#define BTN_SIZE 100
const int BTN1_X = 30, BTN1_Y = 110;
const int BTN2_X = 140, BTN2_Y = 110;

CST820 touch(33, 32, 25, -1); // pines correctos del CYD

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastTouch = 0;
const unsigned long debounce = 300;
bool btn1 = false, btn2 = true;

void connectWiFi() {
  Serial.print("WiFi…");
  WiFi.begin(ssid, password);
  while (WiFi.status()!=WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" listo");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT…");
    if (client.connect("CYDTouch")) Serial.println(" conectado");
    else { Serial.print(" fallo "); Serial.print(client.state()); Serial.println(", retry"); delay(5000); }
  }
}

void drawBtn(int x, int y, bool st, const char* lab) {
  uint16_t bg = st?TFT_GREEN:TFT_DARKGREY;
  uint16_t br = st?TFT_WHITE:TFT_LIGHTGREY;
  tft.fillRoundRect(x,y,BTN_SIZE,BTN_SIZE,15,bg);
  for(int i=0;i<3;i++) tft.drawRoundRect(x+i,y+i,BTN_SIZE-2*i,BTN_SIZE-2*i,15,br);
  tft.setTextColor(TFT_WHITE,bg); tft.setTextSize(2);
  int tbx = x + BTN_SIZE/2 - (6 * strlen(lab));
  int tby = y + BTN_SIZE/2 - 8;
  tft.setCursor(tbx, tby); tft.print(lab);
}

void setup() {
  Serial.begin(115200);
  tft.begin(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);
  drawBtn(BTN1_X,BTN1_Y,btn1, btn1?"Encendido":"Apagado");
  drawBtn(BTN2_X,BTN2_Y,btn2, btn2?"Encendido":"Apagado");
  touch.begin(); connectWiFi();
  client.setServer(mqtt_server,1883);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  uint16_t x,y; uint8_t g;
  if (touch.getTouch(&x,&y,&g) && millis()-lastTouch>debounce) {
    lastTouch = millis();
    Serial.printf("Touch x=%d y=%d\n", x, y);
    bool in1 = x>=BTN1_X && x<=BTN1_X+BTN_SIZE && y>=BTN1_Y && y<=BTN1_Y+BTN_SIZE;
    bool in2 = x>=BTN2_X && x<=BTN2_X+BTN_SIZE && y>=BTN2_Y && y<=BTN2_Y+BTN_SIZE;
    if (in1 && !in2) {
      btn1=!btn1;
      client.publish("touch/button1", btn1?"on":"off");
      Serial.printf("Botón1 %s\n", btn1?"encendido":"apagado");
      drawBtn(BTN1_X,BTN1_Y,btn1, btn1?"Encendido":"Apagado");
    } else if (in2 && !in1) {
      btn2=!btn2;
      client.publish("touch/button2", btn2?"on":"off");
      Serial.printf("Botón2 %s\n", btn2?"encendido":"apagado");
      drawBtn(BTN2_X,BTN2_Y,btn2, btn2?"Encendido":"Apagado");
    }
  }
}
