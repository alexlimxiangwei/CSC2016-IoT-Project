#include <M5StickCPlus.h>
#include <WiFi.h>
#include <BLEDevice.h>

// WiFi and BLE configuration
const char* ssid = "alix";
const char* password = "hottestspot";
const int serverPort = 80;
WiFiServer server(serverPort);
// const char* bleServerName = "M5StickPlus";

// Fall detection threshold and variables
const float FALL_THRESHOLD = 5; // Adjust this value based on your requirements
bool fallDetected = false;

// Step counting variables
const float STEP_THRESHOLD = 0.6; // Adjust this value based on your requirements
int stepCount = 0;
float previousAccelY = 0;
bool stepDetected = false;

// Emergency variable
bool emergency = false;

void setup() {
  M5.begin();
  M5.IMU.Init();
  M5.IMU.SetGyroFsr(M5.IMU.GFS_2000DPS);
  M5.IMU.SetAccelFsr(M5.IMU.AFS_16G);

  // Initialize WiFi and BLE
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.println("Connecting to WiFi...");
  }
  server.begin();
  M5.Lcd.print("Server started. IP: ");
  M5.Lcd.println(WiFi.localIP());

  // BLEDevice::init(bleServerName);
  // Set up BLE server and characteristics
  // ...
}


String getSensorData() {
  M5.update();

  // Check if the main button is pressed
  if (M5.BtnA.wasPressed()) {
    emergency = !emergency;
  }

  // Fall detection
  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);
  if (sqrt(ax*ax + ay*ay + az*az) > FALL_THRESHOLD) {
    fallDetected = true;
  }

  // Step counting algorithm
  float accelY = ay;
  float accelDiff = accelY - previousAccelY;
  if (accelDiff > STEP_THRESHOLD && !stepDetected) {
    stepDetected = true;
    stepCount++;
  } else if (accelDiff < -STEP_THRESHOLD) {
    stepDetected = false;
  }
  previousAccelY = accelY;

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "Content-Type: text/plain\r\n\r\n";
  response += "Acceleration: " + String(ax) + ", " + String(ay) + ", " + String(az) + "\n";
  response += "Step Count: " + String(stepCount) + "\n";
  response += "Fall Detected: " + String(fallDetected) + "\n";
  response += "Emergency: " + String(emergency);
  return response;
}


void sendResponse(WiFiClient& client, const String& response, const String& contentType = "text/plain", int statusCode = 200) {
  client.print("HTTP/1.1 " + String(statusCode) + " OK\r\n");
  client.print("Access-Control-Allow-Origin: *\r\n");
  client.print("Content-Type: " + contentType + "\r\n\r\n");
  client.print(response);
  delay(100);
  client.stop();
}


void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.startsWith("GET /data")) {
      // Handle GET request for sensor data
      String response = getSensorData();
      sendResponse(client, response);
    } else if (request.startsWith("POST /emergency")) {
      // Handle POST request to toggle emergency state
      emergency = false;
      String response = "Emergency state: " + String(emergency);
      sendResponse(client, response);
    } else if (request.startsWith("POST /fall")) {
      // Handle POST request to toggle emergency state
      fallDetected = false;
      String response = "fallDetected state: " + String(fallDetected);
      M5.Lcd.print("fallDetected state: " + String(fallDetected));
      sendResponse(client, response);
    } else {
      // Handle invalid requests
      String response = "Invalid request";
      sendResponse(client, response, "text/plain", 400);
    }
  }
}