#include <Wire.h>
#include <SPI.h>
#include "Arduino_GigaDisplay_GFX.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiSSLClient.h>
#include <string>
#include "USBHostGiga.h"


GigaDisplay_GFX display; // create the object
#define BLACK 0x0000
#define ARDUINOJSON_DECODE_UNICODE 1

Keyboard keyb;
HostSerial ser;

char server[] = "api.openai.com";     
const int serverPort = 443;

char ssid[] = "SSID_NETWORK_NAME";        
char pass[] = "WIFI_PASSWORD";        
std::string apikey = "OPENAI_APIKEY"; 
int status = WL_IDLE_STATUS;

String getUserInput() {
  display.fillScreen(BLACK);
  display.setCursor(10, 10);
  display.println("User Prompt");
  display.setCursor(40, 40);

  String userInput = "";
  while (true) {
    if (keyb.available()) {
      auto _key = keyb.read();
      char inputChar = keyb.getAscii(_key);
      display.print(inputChar);
      if (inputChar == '1') {
        break;
      } else {
        userInput += inputChar;
      }
    }
  }

  return userInput;
}


void setup() {

  Serial.begin(115200);
  while (!Serial);
  pinMode(PA_15, OUTPUT);
  keyb.begin();
  ser.begin();
  display.begin();
  display.fillScreen(BLACK);
  display.setCursor(0, 0);
  display.setTextSize(3);
  display.setRotation(3);
  delay(1000);
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  while (status != WL_CONNECTED) {
    delay(1000);
    display.fillScreen(BLACK);
    display.setCursor(10, 10);
    display.println("Accessing Wi-Fi...");
    Serial.println("Accessing Wi-Fi...");
    status = WiFi.begin(ssid, pass);
    delay(3000);
  }
  delay(1000);
  display.fillScreen(BLACK);
  display.setCursor(10, 10);
  display.println("Connected to Wi-Fi");
  display.setCursor(40, 40);
  display.println(ssid);
  Serial.println("Connected to Wi-Fi");
  Serial.println(ssid);
  delay(1000);

}

void loop() {
  String userPrompt = getUserInput();
  Serial.println(userPrompt);
   // Create the JSON payload using ArduinoJson library
  DynamicJsonDocument doc(1024);
  doc["model"] = "gpt-3.5-turbo";
  doc["temperature"] = 0.7;
  doc["stream"] = true;

  JsonArray messages = doc.createNestedArray("messages");

  JsonObject systemMessage = messages.createNestedObject();
  systemMessage["role"] = "system";
  systemMessage["content"] = "You are an assistant. You have a character limit of about 100 words ";

  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = userPrompt;

  String requestBody;
  serializeJson(doc, requestBody);

  delay(3000);
  Serial.println("\nStarting connection to server...");

  WiFiSSLClient client;

  if (client.connect(server, serverPort)) {
    Serial.println(client.status());
    Serial.println("Connected to the Server: api.openai.com");
    delay(100);

    // Send HTTP POST request and change the prompt to your own
    String request = "POST /v1/chat/completions HTTP/1.1\r\n";
    request += "Host: api.openai.com\r\n";
    request += "Authorization: Bearer " + String(apikey.c_str()) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(requestBody.length()) + "\r\n";
    request += "Connection: close\r\n\r\n";
    request += requestBody;

  
    Serial.println(request);
    client.print(request);
    String response;
    String accumulatedContent; // Variable to accumulate content words

    while (client.connected()) {
      String line = client.readStringUntil('\n');
   
      if (line.startsWith("data: ")) {
            // Remove the "data: " prefix
            line.remove(0, 6);
            Serial.println(line);
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, line);  

            if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            display.fillScreen(BLACK);
            display.setCursor(10, 10);
            display.print("Failed to parse JSON: ");
            display.println(error.c_str());
        } 
        else if   (line.equals("[DONE]")) {
            break;
      }
        else {
            JsonArray choices = doc["choices"];
            for (JsonObject choice : choices) {
              String content = choice["delta"]["content"];
              Serial.println("ChatGPT Response: " + content);
              accumulatedContent += content + " ";
    
              display.fillScreen(BLACK);
              display.setCursor(10, 10);
              display.println("ChatGPT Response:");
              display.setCursor(40, 40);
              display.println(accumulatedContent);
            }
        }     
    }
    }

  } else {
    Serial.println("api.openai.com connection failed");
    display.fillScreen(BLACK);
    display.setCursor(10, 10);
    display.println("api.openai.com connection failed");
  }

    // Delay between iterations
  delay(10000);
}