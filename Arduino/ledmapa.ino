#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>  // https://github.com/adafruit/Adafruit_NeoPixel
#include <ArduinoJson.h>        // https://arduinojson.org/
#include <Adafruit_BMP280.h>
#include <esp_task_wdt.h>

#define WDT_TIMEOUT 10

const int numMesta = 72;
const int numOkraj = 28;

const int intervalBlik = 3000;

int ledBuffer[numOkraj];  // which LED are we talking to?
float ledSpeed[numOkraj]; // how fast is this one fading?
int ledDir[numOkraj]; // what direction is the LED going?
int ledMax[numOkraj]; // led max brightness
int ledDeathCount[numOkraj]; // is the light on or off?

int arrBlik[numMesta];
int arrBlikOd[numMesta];
int arrBlikDo[numMesta];

uint32_t arrMesta[numMesta];
uint32_t arrOkraj[numOkraj];

unsigned long now;
unsigned long blik_ms;

// scale the rgb levels for a yellowish tint
float sp_r = 0.;
float sp_g = 0.;
float sp_b = 1.;
int sp_delay = 20;
int sp_bri = 20;
int sp_dea = 100;

int fade_dir = 1;
int fade_bright = 100;
int fade_hue = 0;
int fade_sat = 255;
int fade_min = 1;
int fade_max = 100;
int fade_delay = 10;
int fade_led_bri = 30;

int jas_mesta = 5;
int jas_okraj = 10;

// Nazev a heslo Wi-Fi
const char *ssid = "DOPLN_SSSID";
const char *heslo = "DOPLN_HESLO";

int chyba;
int intEfekt_okraj = 0;

Adafruit_NeoPixel pix_mesta(numMesta, 25, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pix_okraj(numOkraj, 13, NEO_GRB + NEO_KHZ800);

WebServer server(80);

StaticJsonDocument<10000> doc;

Adafruit_BMP280 bmp;

void sp_update(int led) {
  // any value in deathCount indicates the LED is dead
  if (ledDeathCount[led] != 0) { // if it's dead
    ledDeathCount[led]++;  // keep dying
    // higher numbers = fewer lights on at a time
    if (ledDeathCount[led] == sp_dea) { // if it has been dead for awhile
      ledDeathCount[led] = 0; // bring it back to life
    }
  } else {
    if (ledBuffer[led] > ledMax[led]) { // if the current value is greater than the max
      ledDir[led] = -1; // start the fade
    }
    if (ledBuffer[led] == 0) { // if the value is 0
      ledDeathCount[led] = 1; // LED is dead
      ledSpeed[led] = float(random(5, 75) / 100.); // give it a new speed
      ledMax[led] = ledSpeed[led] * 255; // a new max value based on the speed
      ledDir[led] = 1; // and reset the direction
    }
    ledBuffer[led] = ledBuffer[led] + ledDir[led] + ledSpeed[led]; // set the value of the LED in the array
  }
}

void fade_update() {
  uint32_t hsv = pix_okraj.ColorHSV(fade_hue,fade_sat,fade_bright);
  pix_okraj.setBrightness(fade_led_bri);
  pix_okraj.fill(hsv);
  pix_okraj.show();
  fade_bright = fade_bright + fade_dir;
  if(fade_bright > fade_max || fade_bright < fade_min) fade_dir = fade_dir * -1;
}

void httpTeplota(void) {
  float tempC = bmp.readTemperature();
  server.send(200, "text/plain", String(tempC));
}

void httpDotaz(void) {
  chyba = 1;

  if (server.hasArg("mesta")) {
    if (server.hasArg("jas")) {
      jas_mesta = server.arg("jas").toInt();
      pix_mesta.setBrightness(jas_mesta);
      pix_mesta.show();
    }
    DeserializationError e = deserializeJson(doc, server.arg("mesta"));
    if (e) {
      if (e == DeserializationError::InvalidInput) {
        server.send(200, "text/plain", "CHYBA\nSpatny format JSON TEPLOT");
      } else if (e == DeserializationError::NoMemory) {
        server.send(200, "text/plain", "CHYBA\nMalo pameti RAM pro JSON TEPLOT. Navys ji!");
      } else {
        server.send(200, "text/plain", "CHYBA\nNepodarilo se mi dekodovat JSON TEPLOT");
      }
    } else {
      chyba = 0;
      pix_mesta.clear();
      JsonArray mesta = doc.as<JsonArray>();
      memset(arrBlik, 0, sizeof(arrBlik));
      memset(arrMesta, 0, sizeof(arrMesta));
      for (JsonObject mesto : mesta) {
        int id = mesto["id"];
        int r = mesto["r"];
        int g = mesto["g"];
        int b = mesto["b"];
        arrBlik[id] = mesto["x"];
        arrMesta[id] = pix_mesta.Color(r, g, b);
        pix_mesta.setPixelColor(id, arrMesta[id]);
      }
      pix_mesta.show();
    }
  } else if (server.hasArg("okraj")) {
    if (server.hasArg("jas")) {
      jas_okraj = server.arg("jas").toInt();
      pix_okraj.setBrightness(jas_okraj);
      pix_okraj.show();
    }
    intEfekt_okraj = 0;
    DeserializationError e = deserializeJson(doc, server.arg("okraj"));
    if (e) {
      if (e == DeserializationError::InvalidInput) {
        server.send(200, "text/plain", "CHYBA\nSpatny format JSON OKRAJE");
      } else if (e == DeserializationError::NoMemory) {
        server.send(200, "text/plain", "CHYBA\nMalo pameti RAM pro JSON OKRAJE. Navys ji!");
      } else {
        server.send(200, "text/plain", "CHYBA\nNepodarilo se mi dekodovat JSON OKRAJE");
      }
    } else {
      chyba = 0;
      pix_okraj.clear();
      memset(arrOkraj, 0, sizeof(arrOkraj));
      JsonArray okraj = doc.as<JsonArray>();
      for (JsonObject pozice : okraj) {
        int id = pozice["id"];
        int r = pozice["r"];
        int g = pozice["g"];
        int b = pozice["b"];
        arrOkraj[id] = pix_okraj.Color(r, g, b);
        pix_okraj.setPixelColor(id, arrOkraj[id]);
      }
      pix_okraj.show();
    }
  } else {
    if (server.hasArg("smazat_mesta")) {
      chyba = 0;
      pix_mesta.clear();
      pix_mesta.show();
    }
    if (server.hasArg("smazat_okraj")) {
      chyba = 0;
      intEfekt_okraj = 0;
      pix_okraj.clear();
      pix_okraj.show();
    }
    if (server.hasArg("efekt_okraj")) {
      DeserializationError e = deserializeJson(doc, server.arg("efekt_okraj"));
      if (e) {
        server.send(200, "text/plain", "CHYBA\nSpatny format JSON");
      } else {
        chyba = 0;
        JsonObject efekt = doc.as<JsonObject>();
        intEfekt_okraj = efekt["id"];
        if (intEfekt_okraj == 1) {
          sp_r = efekt["r"];
          sp_g = efekt["g"];
          sp_b = efekt["b"];
          sp_bri = efekt["bri"];
          sp_delay = efekt["del"];
          sp_dea = efekt["dea"];
        } else if (intEfekt_okraj == 2) {
          fade_hue = efekt["hue"];
          fade_sat = efekt["sat"];
          fade_min = efekt["min"];
          fade_max = efekt["max"];
          fade_delay = efekt["del"];
          fade_led_bri = efekt["bri"];
        }
      }
    }
  }
  if (chyba == 0) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "CHYBA\nNeznamy prikaz");
  }
}

void setup() {

  Serial.begin(115200);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, heslo);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  server.on("/", httpDotaz);
  server.on("/teplota", httpTeplota);
  server.begin();

  pix_mesta.begin();
  pix_mesta.setBrightness(jas_mesta);
  pix_mesta.clear();
  pix_mesta.show();

  pix_okraj.begin();
  pix_okraj.setBrightness(jas_okraj);
  pix_okraj.clear();
  pix_okraj.show();

  for (int i = 0; i < numOkraj; i++) {
    ledSpeed[i] = float(random(5, 75) / 100.);
    ledMax[i] = ledSpeed[i] * 255;
    ledDir[i] = 1;
    ledDeathCount[i] = 0;
  }

  unsigned status;
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor!"));
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X4,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_NONE,   /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  blik_ms = millis();
}

void loop() {
  now = millis();

  if (intEfekt_okraj == 1) {
    for (int i = 0; i < numOkraj; i++) {
      sp_update(i);
    }
    for (int i = 0; i < numOkraj; i++) {
      pix_okraj.setPixelColor(i, int(ledBuffer[i] * sp_r), int(ledBuffer[i] * sp_g), int(ledBuffer[i] * sp_b));
    }
    pix_okraj.setBrightness(sp_bri);
    pix_okraj.show();
    delay(sp_delay);
  } else if (intEfekt_okraj == 2) {
    fade_update();
    delay(fade_delay);
  }  
  /////////////////////////////////
  if (now > (blik_ms + intervalBlik)) {
    blik_ms = now;
    for (int i = 0; i < numMesta; i++) {
      if (arrBlik[i] == 1) {
        arrBlikOd[i] = now + random(100,800);
        arrBlikDo[i] = arrBlikOd[i] + 100;
      }
    }
  }
  for (int i = 0; i < numMesta; i++) {
    if (arrBlikOd[i] > 0 && now > arrBlikOd[i]) {
      arrBlikOd[i] = 0;
      pix_mesta.setPixelColor(i,255,255,255);
      pix_mesta.show();
    }
    if (arrBlikDo[i] > 0 && now > arrBlikDo[i]) {
      arrBlikDo[i] = 0;
      pix_mesta.setPixelColor(i, arrMesta[i]);
      pix_mesta.show();
    }
  }
  //}
  /////////////////////////////////
  server.handleClient();
  esp_task_wdt_reset();
  delay(2);
}
