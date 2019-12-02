#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#include <WiFiMan.h>

#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 12
#define DHTTYPE DHT11

#define RELAYPIN 14

#define MQTT_IP "192.168.1.11"
#define MQTT_PORT 1883

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
Config conf;

float desiredTemp;
float sensorTemp;
boolean compressor;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_IP, MQTT_PORT);

Adafruit_MQTT_Publish sensorTempPub = Adafruit_MQTT_Publish(&mqtt, "keezer0/feeds/sensorTemp");
Adafruit_MQTT_Publish compressorPub = Adafruit_MQTT_Publish(&mqtt, "keezer0/feeds/compressor");
Adafruit_MQTT_Subscribe desiredTempSub = Adafruit_MQTT_Subscribe(&mqtt, "keezer0/feeds/desiredTemp");

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, LOW);
    
  // Initialize device.
  dht.begin();
  Serial.println(F("Keezer v0.0.1 developed by Graham Mackie 2019-02-04"));

  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  WiFiMan wman = WiFiMan();
  wman.setHelpInfo("This is where the help would go...");
  wman.setApName("keezer");
  wman.setApPasswd("keezerpass");
  wman.start();
  if(wman.getConfig(&conf))
  {
    //display device status
    Serial.println("Connected to AP");
    Serial.print("SSID : ");
    Serial.println(conf.wifiSsid);
    Serial.print("Passwd : ");
    Serial.println(conf.wifiPasswd);
    Serial.print("MQTT server : ");
    Serial.println(conf.mqttAddr);
    Serial.print("MQTT password : ");
    Serial.println(conf.mqttPasswd);
    Serial.print("MQTT username : ");
    Serial.println(conf.mqttUsername);
    Serial.print("MQTT Id : ");
    Serial.println(conf.mqttId);
    Serial.print("Sub topic : ");
    Serial.println(conf.mqttSub);
    Serial.print("Pub topic : ");
    Serial.println(conf.mqttPub);
    Serial.print("MQTT port : ");
    Serial.println(conf.mqttPort);
    Serial.print("Master password : ");
    Serial.println(conf.masterPasswd);
    Serial.print("Device mDNS name : ");
    Serial.println(conf.mdnsName);
    Serial.print("IP : ");
    Serial.println(conf.localIP);
  }

  mqtt.subscribe(&desiredTempSub);
}

float getDesiredTemp() {
  float temp;
  MQTT_connect();
  
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &desiredTempSub) {
      temp = String((char *)&desiredTempSub.lastread).toFloat();
      Serial.print(F("Desired Temp from MQTT: "));
      Serial.print(temp);
      Serial.println(F("°C"));
    }
  }

  if (!temp) {
    Serial.println(F("Error getting desired temp from MQTT, using default (4°C)"));
    temp = 4.0;
  }
  return temp;
}

boolean evaluateCompressor(float sensorTemp, float desiredTemp) {
  return sensorTemp > desiredTemp;
}

void loop() {
  // Delay between measurements.
  delay(delayMS);
  desiredTemp = getDesiredTemp();
  
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  sensorTemp = event.temperature;
  if (isnan(sensorTemp)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Sensor Temperature: "));
    Serial.print(sensorTemp);
    Serial.println(F("°C"));
    compressor = evaluateCompressor(sensorTemp, desiredTemp);
    if (compressor) {
      digitalWrite(RELAYPIN, HIGH);
      Serial.println(F("Relay Status: ON"));
    }
    else {
      digitalWrite(RELAYPIN, LOW);
      Serial.println(F("Relay Status: OFF"));
    }
    sensorTempPub.publish(sensorTemp);
    compressorPub.publish(compressor);
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
