#include <ESP8266WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>

// Setting up pins
#define DHTPIN 0     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
#define RELAY_PIN 4

// Setting up WiFi network connection
const char* ssid = "TS-5Z5h";
const char* password = "79hXaEPX";

// Setting up MQTT server on Digital Ocean
const char* mqtt_server = "46.101.161.155";
const char* mqtt_username = "milos";
const char* mqtt_password = "perspektiva";

// Configuring clients
WiFiClient espClient;
PubSubClient client(espClient);

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;
// Setting value when to automatically turn off
unsigned long heatingServiceStartTime = 0;
unsigned long timeLimit = 3 * 60 * 60 * 1000; // 3h to ms

void setup() {
  dht.begin();

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop())
    client.connect("ESP8266Client", mqtt_username, mqtt_password);

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);

    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);


    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/humidity", humidityTemp);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
  }

  if (isHeatingServiceActivated()) {
    if (millis() - heatingServiceStartTime > timeLimit) {
      Serial.println("Time limit expired");
      Serial.println("Turning off heating system...");
      digitalWrite(RELAY_PIN, LOW);
      heatingServiceStartTime = 0;
    } else {
      int timeleft = (timeLimit - (millis() - heatingServiceStartTime)) / 1000;
      int s = timeleft % 60;
      int m = (timeleft / 60) % 60;
      int h = (timeleft / 60) / 60;
      String timeleftString = String(h) + " sati " + String(m) + " minuta " + String(s) + " sekundi";

      client.publish("room/time-left", timeleftString.c_str());
    }
  } else {
    client.publish("room/time-left", "");
  }
}

bool isHeatingServiceActivated() {
  return digitalRead(RELAY_PIN) == HIGH;
}

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if (topic == "room/heat") {
    Serial.print("Changing Room Heating status ");
    if (messageTemp == "true") {
      digitalWrite(RELAY_PIN, HIGH);
      heatingServiceStartTime = millis();
      Serial.print("On");
    }
    else if (messageTemp == "false") {
      digitalWrite(RELAY_PIN, LOW);
      heatingServiceStartTime = 0;
      Serial.print("Off");
    }
  }

  if (topic == "room/duration") {
    if (!isHeatingServiceActivated()) {
      timeLimit = messageTemp.toInt() * 60 * 60 * 1000;
      Serial.println("Time limit changed to ");
      Serial.println(timeLimit);
    }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
      YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
      To change the ESP device ID, you will have to give a new name to the ESP8266.
      Here's how it looks:
       if (client.connect("ESP8266Client")) {
      You can do it like this:
       if (client.connect("ESP1_Office")) {
      Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/heat");
      client.subscribe("room/duration");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
