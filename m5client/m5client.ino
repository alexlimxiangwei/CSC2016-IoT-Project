#include <WiFi.h>
#include <HTTPClient.h>
#include <M5StickCPlus.h>

int device_id = 1;

const char* ssid = "Yeetus";
const char* password = "182408092000";
String serverName = "http://192.168.1.211:5000/data/" + String(device_id); // Change to your server's IP and device ID

// Initialize your sensor or other variables here
float ax, ay, az; // Accelerometer values
bool fallDetected = false;
bool emergency = false;
int stepCount = 0;

void setup() {
  M5.begin();
  WiFi.begin(ssid, password);
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0, 2);
    M5.Lcd.println("Connecting to WiFi...");
  }
  M5.Lcd.println("Connected to WiFi");
  M5.IMU.Init();
}

void loop() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("Sending info...");
  // Update sensor values here
  M5.IMU.getAccelData(&ax, &ay, &az);
  // For simplicity, let's just simulate step count and fall detection toggling
  stepCount++;
  fallDetected = !fallDetected;
  emergency = !emergency;

  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    // Construct the JSON data string
    String httpRequestData = "{\"acceleration\":\"" + String(ax) + ", " + String(ay) + ", " + String(az) + "\", ";
    httpRequestData += "\"step_count\":\"" + String(stepCount) + "\", ";
    httpRequestData += "\"fall_detected\":\"" + String(fallDetected) + "\", ";
    httpRequestData += "\"emergency\":\"" + String(emergency) + "\"}";
    // Send the POST request
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      // Successful POST request
      String response = http.getString(); // Get the response to the request
      M5.Lcd.println(response); // Display server response
    } else {
      // POST request failed
      M5.Lcd.println("Error on sending POST: " + String(httpResponseCode));
    }
    http.end();
  }
  delay(10000); // Delay before sending the next POST request
}
