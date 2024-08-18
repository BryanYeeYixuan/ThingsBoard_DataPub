#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "HX711.h"

// Replace with your network credentials
const char* ssid = "Souls";
const char* password = "T6mw-YSej-syim-om4i";

// Replace with your ThingsBoard server and device access token
const char* mqtt_server = "thingsboard.cloud";
//const char* access_token = "MRSM8np16P376Cie6hUs";
const char* access_token = "gY5WoQ2tSotfVJBnqATP";

WiFiClient espClient;
PubSubClient client(espClient);

// Ultrasonic sensor 1 pins
const int trigPin1 = 13;
const int echoPin1 = 12;

// Ultrasonic sensor 2 pins
const int trigPin2 = 21;
const int echoPin2 = 19;

// DHT11 sensor pin and type
#define DHTPIN 22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

// Water level sensor pins (ADC1 pins)
const int waterSensorPin = 34;
const int waterSensorPin2 = 33; // New water sensor pin

// Photoresistor sensor pin
const int photoresistorPin = 32; // Use an appropriate analog pin

// HX711 load cell pins
const int LOADCELL_DOUT_PIN = 15;
const int LOADCELL_SCK_PIN = 2;

HX711 scale;
float calibration_factor = 2280.0;  // Replace with your own calibration factor
float last_known_weight = 0;

long duration1;
float distanceCm1;
float distanceInch1;

long duration2;
float distanceCm2;
float distanceInch2;

float humidity;
float temperature;
int waterLevel;
int waterLevel2; // New water sensor value

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", access_token, NULL)) {
      Serial.println("connected");
      // Subscribe to a topic if needed
      // client.subscribe("yourTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); // Starts the serial communication

  // Initialize ultrasonic sensor 1
  pinMode(trigPin1, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin1, INPUT); // Sets the echoPin as an Input

  // Initialize ultrasonic sensor 2
  pinMode(trigPin2, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin2, INPUT); // Sets the echoPin as an Input

  pinMode(waterSensorPin, INPUT); // Sets the waterSensorPin as an Input
  pinMode(waterSensorPin2, INPUT); // Sets the waterSensorPin2 as an Input
  pinMode(photoresistorPin, INPUT); // Sets the photoresistorPin as an Input

  dht.begin(); // Initialize DHT sensor

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);  // Set the predetermined calibration factor
  scale.tare();  // Reset scale to 0

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Wait for a moment to ensure everything is initialized properly
  delay(1000);
}

void printTableHeader() {
  Serial.println(F(""));
  Serial.println(F("PUBLISHING TO: "));
  Serial.println(F("                            THINGSBOARD                            "));
  Serial.println(F("+=====================+=====================+=====================+"));
  Serial.println(F("| Sensor              | Reading             | Units               |"));
  Serial.println(F("+=====================+=====================+=====================+"));
}

void printTableFooter() {
  Serial.println(F("+=====================+=====================+=====================+"));
  Serial.println(F("                            END OF TABLE                           "));
  Serial.println(F(""));
}

void printSensorData(float distanceCm1, float distanceCm2, float humidity, float temperature, int waterLevel1, int waterLevel2, int lightSensor, float weight) {
  char buffer[20];
  Serial.print(F("| Ultrasonic Sensor 1 | "));
  snprintf(buffer, sizeof(buffer), "%-20.2f", distanceCm1);
  Serial.print(buffer);
  Serial.print(F(" | cm                  |"));
  Serial.println();
  
  Serial.print(F("| Ultrasonic Sensor 2 | "));
  snprintf(buffer, sizeof(buffer), "%-20.2f", distanceCm2);
  Serial.print(buffer);
  Serial.print(F(" | cm                  |"));
  Serial.println();
  
  Serial.print(F("| Humidity            | "));
  snprintf(buffer, sizeof(buffer), "%-20.1f", humidity);
  Serial.print(buffer);
  Serial.print(F(" | %                   |"));
  Serial.println();
  
  Serial.print(F("| Temperature         | "));
  snprintf(buffer, sizeof(buffer), "%-20.1f", temperature);
  Serial.print(buffer);
  Serial.print(F(" | °C                  |"));
  Serial.println();
  
  Serial.print(F("| Water Level Sensor 1| "));
  snprintf(buffer, sizeof(buffer), "%-20d", waterLevel1);
  Serial.print(buffer);
  Serial.print(F(" | -                   |"));
  Serial.println();
  
  Serial.print(F("| Water Level Sensor 2| "));
  snprintf(buffer, sizeof(buffer), "%-20d", waterLevel2);
  Serial.print(buffer);
  Serial.print(F(" | -                   |"));
  Serial.println();
  
  Serial.print(F("| Light Sensor        | "));
  snprintf(buffer, sizeof(buffer), "%-20d", lightSensor);
  Serial.print(buffer);
  Serial.print(F(" | -                   |"));
  Serial.println();
  
  Serial.print(F("| Weight              | "));
  snprintf(buffer, sizeof(buffer), "%-20.2f", weight);
  Serial.print(buffer);
  Serial.print(F(" | kg                  |"));
  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ultrasonic sensor 1 readings
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  
  duration1 = pulseIn(echoPin1, HIGH);
  distanceCm1 = 250 - (duration1 * SOUND_SPEED / 2);
  
  // Ultrasonic sensor 2 readings
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  
  duration2 = pulseIn(echoPin2, HIGH);
  distanceCm2 = 250 - (duration2 * SOUND_SPEED / 2);

  // DHT11 sensor readings
  delay(2000); // DHT11 needs at least 1s between reads
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Water level sensor readings
  int waterLevelValue1 = analogRead(waterSensorPin);
  int waterLevelValue2 = analogRead(waterSensorPin2);

  // Photoresistor sensor readings
  int sensorValue = analogRead(photoresistorPin);
  sensorValue = 1023 - sensorValue;
  if (sensorValue < 0) {
    sensorValue = 0;
  }

  // HX711 weight sensor reading
  float weight = scale.get_units(10);

  // Check for weight change
  if (abs(weight - last_known_weight) > 0.01) {
    last_known_weight = weight;
  }

  // Create JSON object
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["ultrasonic1"] = String(distanceCm1, 2);
  jsonDoc["ultrasonic2"] = String(distanceCm2, 2);
  jsonDoc["humidity"] = humidity;
  jsonDoc["temperature"] = temperature;
  jsonDoc["waterlevel1"] = waterLevelValue1;
  jsonDoc["waterlevel2"] = waterLevelValue2;
  jsonDoc["lightsensor"] = sensorValue;
  jsonDoc["weight"] = String(weight, 2);

  // Serialize JSON object to string
  char buffer[256];
  serializeJson(jsonDoc, buffer);

  // Publish JSON string to ThingsBoard
  client.publish("v1/devices/me/telemetry", buffer);

  // Print the sensor data table
  printTableHeader();
  printSensorData(distanceCm1, distanceCm2, humidity, temperature, waterLevelValue1, waterLevelValue2, sensorValue, weight);
  printTableFooter();

  // Wait for a while before taking new readings
  delay(2000);
}
