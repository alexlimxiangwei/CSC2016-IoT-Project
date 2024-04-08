#include <M5StickCPlus.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <PubSubClient.h>

// WiFi and BLE configuration
const char* ssid = "alix";
const char* password = "hottestspot";
const int serverPort = 80;
WiFiServer server(serverPort);
// const char* bleServerName = "M5StickPlus";

// Fall detection threshold and variables
const float FALL_THRESHOLD = 10; // Adjust this value based on your requirements
bool fallDetected = false;

// Emergency variable
bool emergency = false;

// Alert variable
bool alert = false;

// Step counting variables
const float STEP_THRESHOLD = 0.6; // Adjust this value based on your requirements
int stepCount = 0;
float previousAccelY = 0;
bool stepDetected = false;

// MQTT setup
const char* mqttServer = "192.168.116.30";
const int mqttPort = 1883;
const char* registrationTopic = "m5stick/registration";
String deviceName = "M5StickPlus2";
String ipAddress;

WiFiClient espClient;
PubSubClient pubSubClient(espClient);

volatile unsigned long btnPressTime = 0;
bool btnPreviouslyPressed = false;
bool emergencyUpdated = false;

const unsigned long keepAliveInterval = 5000; // 5 seconds
unsigned long lastKeepAliveTime = 0;
unsigned long timeSinceLastStep = 0;

void IRAM_ATTR handleBtnPress() {
  if (!fallDetected){
    emergency = true; // Set emergency to true immediately upon button press
  }
}

void sendKeepAlive() {
    if (millis() - lastKeepAliveTime > keepAliveInterval) {
        ipAddress = WiFi.localIP().toString();
        String message = deviceName + "," + ipAddress;
        pubSubClient.publish(registrationTopic, message.c_str());
        lastKeepAliveTime = millis();

        if (!emergency && !fallDetected){
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.setRotation(3);
            M5.Lcd.setTextSize(2);
            M5.Lcd.setCursor(10, 20);
            M5.Lcd.print("Steps: ");
            M5.Lcd.print(stepCount);
        }
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!pubSubClient.connected()) {
        M5.Lcd.println("Attempting MQTT connection...");
        // Attempt to connect
        if (pubSubClient.connect("M5StickClient")) {
            M5.Lcd.println("MQTT connected");
            // Resubscribe or republish here if necessary
        } else {
            M5.Lcd.print("MQTT connection failed, rc=");
            M5.Lcd.print(pubSubClient.state());
            M5.Lcd.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    ipAddress = WiFi.localIP().toString();
    String message = deviceName + "," + ipAddress;
    pubSubClient.publish(registrationTopic, message.c_str());
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  M5.IMU.SetGyroFsr(M5.IMU.GFS_2000DPS);
  M5.IMU.SetAccelFsr(M5.IMU.AFS_16G);
  pinMode(M5_LED, OUTPUT);
  digitalWrite (M5_LED, HIGH);

  pinMode(M5_BUTTON_HOME, INPUT_PULLUP); // Ensure the button is set to input with pull-up
  attachInterrupt(digitalPinToInterrupt(M5_BUTTON_HOME), handleBtnPress, FALLING); // For button press

  // Initialize WiFi and BLE
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.println("Connecting to WiFi...");
  }
  server.begin();
  M5.Lcd.print("Server started. IP: ");
  M5.Lcd.println(WiFi.localIP());

  pubSubClient.setServer(mqttServer, mqttPort);
  while (!pubSubClient.connected()) {
    if (pubSubClient.connect("M5StickClient")) {
      M5.Lcd.println("Connected to MQTT broker");
    } else {
      M5.Lcd.print("Failed to connect to MQTT broker, rc=");
     M5.Lcd.println(pubSubClient.state());
      delay(1000);
    }
  }

 
  ipAddress = WiFi.localIP().toString();
  String message = deviceName + "," + ipAddress;
  pubSubClient.publish(registrationTopic, message.c_str());
}

void updateFallDetection(){
   // Fall detection
  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);
  if (sqrt(ax*ax + ay*ay + az*az) > FALL_THRESHOLD && !fallDetected) {
    fallDetected = true;
    pubSubClient.publish((ipAddress + "/fall").c_str(), String(fallDetected).c_str());
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 20);
    M5.Lcd.println("FALL");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println("DETECTED");

    digitalWrite (M5_LED, LOW);
  }

  float accelY = ay;
  float accelDiff = accelY - previousAccelY;

  // Define both positive and negative thresholds for detecting a step
  const float POSITIVE_STEP_THRESHOLD = 0.6; // Threshold for the step forward/upward motion
  const float NEGATIVE_STEP_THRESHOLD = -0.6; // Threshold for the step settling/downward motion

  // Track whether we've seen a positive motion, indicating the start of a potential step
  static bool positiveMotionDetected = false;

  if (!stepDetected && (millis() - timeSinceLastStep > 500)) {
      if (accelDiff > POSITIVE_STEP_THRESHOLD) {
          positiveMotionDetected = true;
      } else if (positiveMotionDetected && accelDiff < NEGATIVE_STEP_THRESHOLD) {
          // A step is detected only after a positive motion followed by a negative motion
          stepCount++;
          timeSinceLastStep = millis();
          positiveMotionDetected = false; // Reset for the next step detection
          stepDetected = true; // Optional: use this flag for additional logic as needed
      }
  } else if (stepDetected && accelDiff > NEGATIVE_STEP_THRESHOLD) {
      stepDetected = false; // Ready to detect the next step
  }

  previousAccelY = accelY;

}

String getSensorData() {
  M5.update();

  // Check if the main button is pressed
  // if (M5.BtnA.wasPressed()) {
  //   emergency = !emergency;
  // }

  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);

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
  response += "Alert: " + String(alert);
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

  if (!pubSubClient.connected()) {
        // Reconnect logic here
    } else {
        pubSubClient.loop(); // Make sure to call this regularly to process incoming messages and maintain the connection
        sendKeepAlive();
  }

  //Update Fall and Emergency every loop
  updateFallDetection();

  if (emergency && !emergencyUpdated){

    pubSubClient.publish((ipAddress + "/emergency").c_str(), String(emergency).c_str());

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 20);
    M5.Lcd.println("EMERGENCY");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println("ACTIVATED");

    digitalWrite (M5_LED, LOW);
    emergencyUpdated = true;
  }


  bool btnPressed = digitalRead(M5_BUTTON_HOME) == LOW;
  if (btnPressed) {
      if (!btnPreviouslyPressed) {
        // Button has just been pressed; record the time
        btnPreviouslyPressed = true;
        btnPressTime = millis();
      } else if (millis() - btnPressTime >= 1000) {
        // The button has been held for more than 1 second
        if (emergency || fallDetected) {
          emergency = false; // Deactivate emergency state
          fallDetected = false; // Deactivate fall state
          pubSubClient.publish((ipAddress + "/emergency").c_str(), String(emergency).c_str());
          pubSubClient.publish((ipAddress + "/fall").c_str(), String(fallDetected).c_str());
          emergencyUpdated = false;
          
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setTextColor(WHITE);
          M5.Lcd.setRotation(3);
          M5.Lcd.setTextSize(2);
          M5.Lcd.setCursor(10, 20);
          M5.Lcd.println("Emergency");
          M5.Lcd.setCursor(10, 50);
          M5.Lcd.println("Deactivated");

          delay(1000);
          
          digitalWrite(M5_LED, HIGH); // Optionally, turn off LED or indicator
          // Ensure this block doesn't run repeatedly while the button is held
          while(digitalRead(M5_BUTTON_HOME) == LOW) {
            delay(10); // Minimal delay to prevent blocking
          }
        }
        btnPreviouslyPressed = false; // Reset the flag after handling
      }
    } else {
      btnPreviouslyPressed = false; // Reset the flag if the button was released before 1 second
    }

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
      digitalWrite (M5_LED, HIGH);
      String response = "fallDetected state: " + String(fallDetected);
      sendResponse(client, response);
    } else if (request.startsWith("POST /toggle_alert")) {
      // Handle POST request to toggle alert state
      alert = !alert;
      String response = "Alert state: " + String(alert);
      
      // Toggle the LED light based on the alert state
      if (alert) {
          digitalWrite (M5_LED, LOW);
      } else {
        digitalWrite (M5_LED, HIGH);
      }
      
      sendResponse(client, response);
    } else {
      // Handle invalid requests
      String response = "Invalid request";
      sendResponse(client, response, "text/plain", 400);
    }
  }
}