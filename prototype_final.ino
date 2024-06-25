#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#else
  #error "Board not found"
#endif

#include <Wire.h>
#include <PubSubClient.h>
#include <ErriezDHT22.h>

// Define pin configurations
#define Relay1  16
#define Relay2  4
#define Relay3  2
#define Relay4  15
#define DHT22_PIN 5  // GPIO5 (Labeled as D2 on some ESP8266 boards)
#define SENSOR_PIN 12 // Define sensor pin

// WiFi credentials
const char* ssid = "SSID";
const char* password = "password";
const char* mqtt_server = "91.121.93.94"; // IP address of MQTT broker

// MQTT topics
#define sub1 "sensor/relay/1"
#define sub2 "sensor/relay/2"
#define sub3 "sensor/relay/3"
#define sub4 "sensor/relay/4"
#define humidity_topic "sensor/DHT22/humidity"
#define temperature_celsius_topic "sensor/DHT22/temperature_celsius"
#define motion_topic "sensor/microwave/motion"

// MQTT and WiFi clients
WiFiClient espClient;
PubSubClient client(espClient);
DHT22 dht22(DHT22_PIN);

// Variable for motion detection
int state = LOW; 
int val = 0; 
unsigned long motionDetectedTime = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Print the payload
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Control relays based on received messages
  if (strcmp(topic, sub1) == 0) {
    digitalWrite(Relay1, (char)payload[0] == '1' ? HIGH : LOW);
  } else if (strcmp(topic, sub2) == 0) {
    digitalWrite(Relay2, (char)payload[0] == '1' ? HIGH : LOW); 
  } else if (strcmp(topic, sub3) == 0) {
    digitalWrite(Relay3, (char)payload[0] == '1' ? HIGH : LOW); 
  } else if (strcmp(topic, sub4) == 0) {
    digitalWrite(Relay4, (char)payload[0] == '1' ? HIGH : LOW); 
  } else {
    Serial.println("unsubscribed topic");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(sub1);
      client.subscribe(sub2);
      client.subscribe(sub3);
      client.subscribe(sub4);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHT22 temperature and humidity sensor \n"));

  // Initialize DHT22 sensor
  dht22.begin();
  
  // Initialize WiFi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.publish(motion_topic, "No motion", true); // turned off the dashboard light

  // Initialize pins
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(Relay4, OUTPUT);
  // pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  // Set initial state for relays 
  digitalWrite(Relay1, LOW);
  digitalWrite(Relay2, LOW);
  digitalWrite(Relay3, LOW);
  digitalWrite(Relay4, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Check and publish sensor data
  if (dht22.available()) {
    int16_t temperature = dht22.readTemperature();
    int16_t humidity = dht22.readHumidity();

    if (temperature != ~0) {
      Serial.print(F("Temperature: "));
      Serial.print(temperature / 10);
      Serial.print(F("."));
      Serial.print(temperature % 10);
      Serial.println(F(" *C"));
      client.publish(temperature_celsius_topic, String(temperature / 10).c_str(), true);
    } else {
      Serial.println(F("Temperature: Error"));
    }

    if (humidity != ~0) {
      Serial.print(F("Humidity: "));
      Serial.print(humidity / 10);
      Serial.print(F("."));
      Serial.print(humidity % 10);
      Serial.println(F(" %"));
      client.publish(humidity_topic, String(humidity / 10).c_str(), true);
    } else {
      Serial.println(F("Humidity: Error"));
    }
  }

  // Check and publish motion detection
  val = digitalRead(SENSOR_PIN);
  if (val == HIGH) {
    if (state == LOW) {
      Serial.println("Motion detected!");
      state = HIGH;
      client.publish(motion_topic, "Motion detected!", true);
      digitalWrite(Relay1, HIGH); // turning on speaker, relay off
      motionDetectedTime = millis(); // Record the time when motion was detected
    }
  }

  // Turn off Relay1 after 10 seconds if it was turned on due to motion
  if (state == HIGH && millis() - motionDetectedTime >= 10000) {
    Serial.println("Turning off Relay1 after 10 seconds.");
    digitalWrite(Relay1, LOW); // turning off speaker, relay on
    state = LOW;
    client.publish(motion_topic, "No motion", true); // Publish no motion message
  }
}
