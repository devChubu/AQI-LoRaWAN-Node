/*
 * File: Gateway.cpp
 
 * Project Description: GTC 2025 MAX16163 WESN Demo
 * Author: devChubu
 * Date: 2025-01-24
 * Version: 1.7
 * Dependencies: 
 *   - SPI.h
 *   - LoRa.h
 *   - Wire.h
 *   - SparkFun_RV8803.h
 *   - Adafruit_SHT4x.h
 *   - Adafruit_PM25AQI.h
 * Usage: 
 *   - This code initializes various LoRaWAN , to recieve data from the nodes, 
 *     parse data, and save data locally to a PC.
 */

// Initialize Libraries
#include <SPI.h> // Serial Peripheral Interface
#include <Wire.h> // For I2C Comms
#include <LoRa.h> // LoRa

void setup() {
  Serial.begin(9600); //  Start Serial Monitor for debugging
  while (!Serial);

  Serial.println("LoRa Gateway");

  //Initialize LoRa
  if (!LoRa.begin(923200000)) {
    Serial.println("Failed to initialize LoRa Module");
    while (1);
  }

  Serial.println("LoRa initialized! Gateway Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket(); // Detect incoming packets
  if (packetSize) {
    String receivedData = "";

    // Read the incoming packet byte by byte
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println("Received data: " + receivedData);

    // Parse JSON-like string manually
    if (receivedData.startsWith("{") && receivedData.endsWith("}")) {
      String nodeId = extractValue(receivedData, "node");
      String timestamp = extractValue(receivedData, "time");
      String temperature = extractValue(receivedData, "temp");
      String humidity = extractValue(receivedData, "hum");
      String pm25 = extractValue(receivedData, "pm25");
      String pm10 = extractValue(receivedData, "pm10");
      String aqi = extractValue(receivedData, "aqi");
      String pollutant = extractValue(receivedData, "pollutant");
      String counter = extractValue(receivedData, "count");

      // Print parsed values
      Serial.println("Parsed Data:");
      Serial.println("  Node ID: " + nodeId);
      Serial.println("  Timestamp: " + timestamp);
      Serial.println("  Temperature: " + temperature + " °C");
      Serial.println("  Humidity: " + humidity + " %");
      Serial.println("  PM2.5: " + pm25 + " µg/m³");
      Serial.println("  PM10: " + pm10 + " µg/m³");
      Serial.println("  AQI: " + aqi);
      Serial.println("  Pollutant: " + pollutant);
      Serial.println("  Counter: " + counter);
    } else {
      Serial.println("Invalid data format.");
    }
  }
}

// Function to extract value from JSON-like string
String extractValue(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\"");
  if (keyIndex == -1) return ""; // Key not found

  int colonIndex = json.indexOf(":", keyIndex);
  if (colonIndex == -1) return ""; // Colon not found

  int valueStart = colonIndex + 1;
  char endChar = (json[valueStart] == '"') ? '"' : ','; // Check if value is string or number
  if (endChar == '"') valueStart++; // Skip opening quote for strings

  int valueEnd = json.indexOf(endChar, valueStart);
  if (valueEnd == -1) valueEnd = json.indexOf("}", valueStart); // Handle last value in JSON

  return json.substring(valueStart, valueEnd);
}