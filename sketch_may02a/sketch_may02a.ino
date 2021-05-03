#include <ESP8266WiFi.h>

const char* ssid = "MILUTIN";
const char* password = "krcedin56";
unsigned long startTime;
unsigned long onlineTimeInMS = 3 * 60 * 60 * 1000; // 3h to ms
int value = LOW;

int relayPin = 4; // GPI10
WiFiServer server(80);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Connect to WiFi network
  Serial.println();
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

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {

  // Check if a client has connected
  client = server.available();
  if (!client) {
    // Heating systme has been active for specified time
    // and now it should be turned off
    if (value == HIGH && millis() - startTime > onlineTimeInMS) {
      value = LOW;
      digitalWrite(relayPin, LOW);
    }
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }


  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request
  if (request.indexOf("/RELAY=ON") != -1 && value == LOW)  {
    digitalWrite(relayPin, HIGH);
    startTime = millis();
    value = HIGH;
  }
  if (request.indexOf("/RELAY=OFF") != -1 && value == HIGH)  {
    digitalWrite(relayPin, LOW);
    value = LOW;
  }

  createResponse(value, client);

  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");

}

void createResponse(int value, WiFiClient client) {
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Grejanje je : ");

  if (value == HIGH) {
    client.print("Upaljeno");
  } else {
    client.print("Ugaseno");
  }
  client.println("<br><br>");
  if (value == HIGH) {
    int hours = (((onlineTimeInMS - millis() + startTime)/ 1000) / 60) / 60;
    int minutes = (((onlineTimeInMS - millis() + startTime)/ 1000) / 60) % 60;
    client.println("Preostalo vreme do gasenja je: ");
    client.print(String(hours));
    client.println("h");
    client.print(String(minutes));
    client.println("min");
    client.println("<br><br>");
  }
  client.println("<a href=\"/RELAY=ON\"\"><button>Upali</button></a>");
  client.println("<a href=\"/RELAY=OFF\"\"><button>Ugasi</button></a><br />");
  client.println("</html>");
}
