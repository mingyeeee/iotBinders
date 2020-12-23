/*
Mingye Chen 2020-12-22
Gets accelerometer data and sends it to the raspberry pi via mqtt for data analysis
*/
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"

#define MQTT_MAX_PACKET_SIZE 256

// MPU 6050 setup
MPU6050 accelgyro;

int16_t ax, ay, az, gx, gy, gz;

// Multi Tasking
TaskHandle_t sensor_monitoring;
TaskHandle_t mqtt_task;
// Semaphore for MPU data
SemaphoreHandle_t mpu_semaphore;
// Vars that require the semaphore
static volatile int mpuData[50];
static volatile int mpuIndex = 0;

// Wifi setup
const char *ssid =  mySSID;   // name of your WiFi network
const char *password =  myPASSWORD; // password of the WiFi network

const char *ID = "B1";  // Name of our device, must be unique

IPAddress broker(mqttBrokerIP); // IP address of your MQTT broker eg. 192,168,1,50
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
  int lastModifiedIndex;
  // Read MPU data if semaphore is free
  xSemaphoreTake(mpu_semaphore, portMAX_DELAY);
  Serial.println("------------mqtt task-------------");
  lastModifiedIndex = (mpuIndex - 1 >= 0) ? mpuIndex - 1 : 49;
  int mpuDataLocal[50];
  Serial.print("last index modified: "); Serial.println(lastModifiedIndex);
  Serial.print("mpuData reading : ");
  for(int i = 0; i < 50; i++){
    mpuDataLocal[i] = mpuData[i];
    Serial.print(mpuDataLocal[i]); Serial.print(", ");
  }
  Serial.println(" done read");
  xSemaphoreGive(mpu_semaphore);

  // Rearranging circular array into linear
  int index = (lastModifiedIndex==49)? 0 : lastModifiedIndex++;
  int increment = 0;
  int mpuDataInorder[50];
  Serial.print("inorder data: ");
  while(increment < 50){
    mpuDataInorder[increment] = mpuDataLocal[index];
    Serial.print(mpuDataInorder[increment]);
    Serial.print(", ");
    index++;
    if(index >49) index = 0;
    increment ++;
  }
  Serial.println("done");
  {
  // Response handling/parsing 
  char mpuJson[128];
  char temp[128] = "{\"B\":1,\"P\":1,\"data\":[";
  for(int i = 0; i < 24; i++){
    sprintf(mpuJson, "%s%d,", temp, mpuDataInorder[i]);
    sprintf(temp, "%s", mpuJson);
  }
  sprintf(mpuJson, "%s%d]}", temp, mpuDataInorder[24]);
  client.publish("binderMotion", mpuJson);
  delay(100);
  }
  {
  // Second Json package
  char mpuJson[128];
  char temp[128] = "{\"B\":1,\"P\":2,\"data\":[";
  for(int i = 25; i < 49; i++){
    sprintf(mpuJson, "%s%d,", temp, mpuDataInorder[i]);
    sprintf(temp, "%s", mpuJson);
  }
  sprintf(mpuJson, "%s%d]}", temp, mpuDataInorder[49]);
  client.publish("binderMotion", mpuJson);
  }
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
  Wire.begin();
  Serial.begin(115200); 

  // Initialize device
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // Verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  //------------MQTT client setup-------------
  delay(100);
  setup_wifi(); // Connect to network
  client.setServer(broker, 1883);
  client.setCallback(callback);// Initialize the callback routine
  
  // Create semaphore for MPU data
  mpu_semaphore = xSemaphoreCreateMutex();
  
  //create a task that will be executed in the sensor_monitoringCode() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    sensor_monitoring_code,   /* Task function. */
                    "sensor_monitoring",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &sensor_monitoring,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the mqtt_taskCode() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    mqtt_task_code,   /* Task function. */
                    "mqtt_task",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &mqtt_task,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 1 */
    delay(500); 
}

void sensor_monitoring_code( void * pvParameters ){
  Serial.print("sensor_monitoring_code running on core ");
  Serial.println(xPortGetCoreID());
  delay(500);
  
  while(1){
    // Write MPU data if semaphore is free
    xSemaphoreTake(mpu_semaphore, portMAX_DELAY);
    Serial.println("------------sensor task-------------");
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    mpuData[mpuIndex] = ((int)ax/1000)+42;
    Serial.print("mpuData ["); Serial.print(mpuIndex); Serial.print("] : "); Serial.println(mpuData[mpuIndex]);
    // Increment index circularly
    mpuIndex = (mpuIndex == 49) ?  0 : mpuIndex + 1;
    xSemaphoreGive(mpu_semaphore);
    vTaskDelay(100);
  }
}

void mqtt_task_code( void * pvParameters ){
  Serial.print("mqtt_task running on core ");
  Serial.println(xPortGetCoreID());
  while(1){
    //Serial.println("mqtt wifi task running");
    if (!client.connected()) reconnect();
    client.loop();
    vTaskDelay(50);
  }
}

void loop() {
  vTaskDelete(NULL);
}
