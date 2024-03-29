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

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();
    M5.Lcd.printf("Step Count: %d", stepCount);
    
    if (request.indexOf("/data") != -1) {
      M5.update();
      
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

      // Location tracking
      // float heading = M5.IMU.getCompassHeading();

      String response = "HTTP/1.1 200 OK\r\n";
      response += "Access-Control-Allow-Origin: *\r\n";
      response += "Content-Type: text/plain\r\n\r\n";

      response += "Acceleration: " + String(ax) + ", " + String(ay) + ", " + String(az) + "\n";
      response += "Step Count: " + String(stepCount) + "\n";
      // response += "Heading: " + String(heading) + "\n";
      response += "Fall Detected: " + String(fallDetected);
      client.print(response);

      delay(100);
    }
    client.stop();
  }
}