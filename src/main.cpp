#include <Arduino.h>

#include <OneWire.h>
#include <DS18B20.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCL D1
#define SDA D2

#define SENSOR_PIN D7
#define RELAY_PIN D8

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

OneWire oneWire(SENSOR_PIN);
DS18B20 sensor(&oneWire);

bool relayOn = false;

void setup() {
  // put your setup code here, to run once:
  sensor.begin();
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:
  sensor.requestTemperatures();

  delay(2000);
  
  float temp = sensor.getTempC();
  relayOn = temp > 15;
  digitalWrite(RELAY_PIN, relayOn);
}