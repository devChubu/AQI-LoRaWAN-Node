#include <Wire.h> // For GPIO Comms
#include <SPI.h> // Serial Peripheral Interface
#include <LoRa.h> // LoRa Library

// Definitions and Pin Assignments
#define CURRENT_SENSE_PIN A1 // Analog input pin connected to the MAX44284 output
#define loadV A2 // Analog input pin connected to the MAX44284 load
#define senseR 0.05 // Rsense, shunt resistor value (in ohms)
#define gainV 100 // MAX44284H gain of 100V/V

// Define the node ID
#define NODE_ID "teleNode 1"

// Define the counter variable
int counter = 0;

void setup() {
  // Initialize serial communication for debugging
  //Serial.begin(9600);
  //while (!Serial);

  // Initialize I2C
  Wire.begin();

  // Initialize LoRa
  if (!LoRa.begin(923200000)) {
    Serial.println("Failed to initialize LoRa Module");
    while (1);
  }
}

void loop() {
  // Read the battery voltage from the MAX44284 load
  int loadADC = analogRead(loadV);
  float battV = loadADC * (3.3 / 1023.0) * 2; // Convert the analog reading to voltage (assuming 3.3V reference) 

  // Read the output voltage from the MAX44284
  int outADC = analogRead(CURRENT_SENSE_PIN);
  float outV = outADC * (3.3 / 1023.0); // Convert the analog reading to voltage (assuming 3.3V reference)

  // Conditional calibration check for outV
  if (outV < 0.02) {
    outV /= 7.425523183;
  }

  // Calculate the current using the output voltage and shunt resistor value
  float loadI = outV / (senseR * gainV);
  float loadI_mA = loadI * 1000; // Convert current to mA

  // Calculate the power consumption of the node
  float loadP = battV * loadI;
  float loadP_mW = loadP * 1000; // Convert power to mW

  // Print the current for debugging
  // Serial.print("Load Current: ");
  // Serial.print(loadI_mA, 5);
  // Serial.println(" mA");
  // Serial.print("Battery Voltage: ");
  // Serial.print(battV, 5);
  // Serial.println(" V");
  // Serial.print("Power Consumption: ");
  // Serial.print(loadP_mW, 5);
  // Serial.println(" mW");

  // Create JSON data packet
  String dataPacket = "{";
  dataPacket += "\"node\":\"" + String(NODE_ID) + "\",";
  dataPacket += "\"out_voltage_V\":" + String(outV, 5) + ",";
  dataPacket += "\"load_current_mA\":" + String(loadI_mA, 5) + ",";
  dataPacket += "\"battery_voltage_V\":" + String(battV, 5) + ",";
  dataPacket += "\"power_consumption_mW\":" + String(loadP_mW, 5);
  dataPacket += "}";

  // Send data to the gateway
  LoRa.beginPacket();
  LoRa.print(dataPacket);
  LoRa.endPacket();

  // Increment the counter
  counter++;
  delay(1000);
}