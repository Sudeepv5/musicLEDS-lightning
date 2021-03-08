#define BLYNK_PRINT Serial
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "sampleCounter.h"
#include "secret.h"

#define LED_PIN D5
#define LED_COUNT 72
#define LED_BRIGHTNESS_DEFAULT 50
#define LED_BRIGHTNESS_MAX 85
#define LED_BRIGHTNESS_MIN 10
#define MIC_LOW 0
#define MIC_HIGH 644
#define LEVEL_LOW 360
#define LEVEL_HIGH 590
#define SAMPLE_SIZE 25
#define LONG_TERM_SAMPLES 140
#define BUFFER_DEVIATION 400
#define BUFFER_SIZE 3

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

struct sampleCounter *samples;
struct sampleCounter *longTermSamples;
struct sampleCounter *sanityBuffer;

struct thunder_data {
  uint32_t data;
};

struct thunder_data thunderData;
float globalHue = 30;
float hueIncrement = 30;

int hueOffset = 60;
int red_value = 0;
int green_value = 0;
int blue_value = 0;
int mode_value = 0;
int led_brightness = LED_BRIGHTNESS_DEFAULT;
float fadeScale = 1.15;


BlynkTimer lightning;
BlynkTimer thunder;
WiFiUDP Udp;

BLYNK_WRITE(V2)
{
  mode_value = param.asInt();
  if (mode_value) {
    bulbMode();
  }
}

BLYNK_WRITE(V3)
{
  red_value = param.asInt();
  if (mode_value) {
    bulbMode();
  }
}

BLYNK_WRITE(V4)
{
  green_value = param.asInt();
  if (mode_value) {
    bulbMode();
  }
}

BLYNK_WRITE(V5)
{
  blue_value = param.asInt();
  if (mode_value) {
    bulbMode();
  }
}

BLYNK_WRITE(V6)
{
  led_brightness = map(param.asInt(), LED_BRIGHTNESS_MIN, LED_BRIGHTNESS_MAX, LED_BRIGHTNESS_MIN, LED_BRIGHTNESS_MAX);
  strip.setBrightness(led_brightness);
  lightning.run();
}

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS_DEFAULT);
  strip.show();
  samples = new sampleCounter(SAMPLE_SIZE);
  longTermSamples = new sampleCounter(LONG_TERM_SAMPLES);
  sanityBuffer    = new sampleCounter(BUFFER_SIZE);
  while(sanityBuffer->setSample(250) == true) {}
  while (longTermSamples->setSample(200) == true) {}
  Blynk.begin(auth, ssid, pass);
  lightning.setInterval(5L, showlightning);
  thunder.setInterval(10L, lightningUpdate);
  Serial.println(WiFi.localIP());
  Udp.begin(8888);

  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
}

void loop() {
    Blynk.run();
    thunder.run();
}
void showlightning() {
  strip.show();
}


void lightningUpdate() {
  if(mode_value) {
    return;
  }

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Udp.read((char *)&thunderData, sizeof(struct thunder_data));
  }
  getLightning(thunderData.data);
  ligthIt(thunderData.data);
  lightning.run();
}

void ligthIt(uint32_t analogRaw) {
  int curshow = map(analogRaw, LEVEL_LOW, LEVEL_HIGH, 0, LED_COUNT);
  for (int i = 0; i < LED_COUNT; i++) {
    if (i <= curshow) {
      int pixelHue = (globalHue + hueOffset + i) * 65536L / strip.numPixels();
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    } else {
      uint8_t LEDr =(strip.getPixelColor(i) >> 16);
      uint8_t LEDg =(strip.getPixelColor(i) >> 8);
      uint8_t LEDb =(strip.getPixelColor(i)) ;
      strip.setPixelColor(i, LEDr/ fadeScale, LEDg/ fadeScale, LEDb/ fadeScale);
    }
  }
}

void getLightning(uint32_t analogRaw) {
    if (analogRaw < 3) {
        return;
    }
    int sanityValue = sanityBuffer->computeAverage();
    if (abs(analogRaw - sanityValue) <= BUFFER_DEVIATION) {
        sanityBuffer->setSample(analogRaw);
    }
    if (samples->setSample(analogRaw)) {
        return;
    }
    uint16_t longTermAverage = longTermSamples->computeAverage();
    uint16_t useVal = samples->computeAverage();
    longTermSamples->setSample(useVal);

    int diff = (useVal - longTermAverage);
    if (diff > 5) {
        if (globalHue < 235) {
            globalHue += hueIncrement;
        }
    } else if (diff < -5) {
        if (globalHue > 2) {
            globalHue -= hueIncrement;
        }
    }
}

void bulbMode() {
    for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, red_value, green_value, blue_value);
    }
    lightning.run();
}
void clearStrip() {
    for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, 0, 0, 0);
    }
    lightning.run();
}