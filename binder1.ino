
#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid =  "Backpack2";   // name of your WiFi network
const char *password =  "iotbinder"; // password of the WiFi network

const byte SWITCH_PIN = 0;           // Pin to control the light with
const char *ID = "Binder1";  // Name of our device, must be unique
const char *TOPIC = "binderStatus";  // Topic to subcribe to

IPAddress broker(192,168,4,1); // IP address of your MQTT broker eg. 192.168.1.50
WiFiClient wclient;

PubSubClient client(wclient); // Setup MQTT client
bool state=0;
// Handle incomming messages from the broker
void callback(char* topic, byte* payload, unsigned int length) {
  
  //---------------------------------------------------------------------
  //turns the incoming payload into a string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  //prints message
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);
  //replies
  client.publish(TOPIC, ID);
  delay(5000);
  //---------------------------------------------------------------------
}
// Connect to WiFi network
void setup_wifi() {
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Connect to network

  while (WiFi.status() != WL_CONNECTED) { // Wait for connection
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Reconnect to client
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID)) {
      Serial.println("connected");
      client.subscribe("piMotion");
      Serial.println('\n');

    } else {
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud
  delay(100);
  setup_wifi(); // Connect to network
  client.setServer(broker, 1883);
  client.setCallback(callback);// Initialize the callback routine
}

void loop() {
  if (!client.connected())  // Reconnect if connection is lost
  {
    reconnect();
  }
  client.loop();
}
