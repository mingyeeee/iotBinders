#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
MPU6050 accelgyro;

int16_t ax, ay, az;

// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
#define OUTPUT_READABLE_ACCELGYRO
//--------------------^mpu stuff-------------------------

#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid =  "BELL652";   // name of your WiFi network
const char *password =  "25594ECFF7F7";//"iotbinder"; // password of the WiFi network

const char *ID = "Binder1";  // Name of our device, must be unique

IPAddress broker(192,168,2,131); // IP address of your MQTT broker eg. 192.168.1.50
WiFiClient wclient;

PubSubClient client(wclient); // Setup MQTT client
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
  char motion[50];
  for (int i=0; i<50; i++){
    accelgyro.getAcceleration(&ax, &ay, &az);
    motion[i] = char(int(ax)/1000);
    Serial.println(int(ax)/1000); Serial.print(" ");
    delay(50);
  }
  delay(1000);
/*
  String motionData = "binder1 ";
  char temp;
  for (int i=0; i<50; i++){
    temp = motion[i];
    motionData = String(temp) + ",";
    //strcat(biMotion, space);
  }*/
  
  client.publish("binderMotion", "ae");
  delay(500);
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
  // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    Serial.begin(115200);

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
//-----------------------------------^mpu stuff-----------------------------
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
