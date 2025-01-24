// Initialize Libraries
#include <SPI.h> // Serial Peripheral Interface
#include <LoRa.h> // LoRa
#include <Wire.h> // For I2C Comms
#include <SparkFun_RV8803.h> // RV-8803 RTC
#include <Adafruit_SHT4x.h> // SHT45 RHT Sensor
#include <Adafruit_PM25AQI.h> // PMSA003I AQI Sensor
#include <utility> // Include this header for std::pair and std::make_pair

#define NODE_ID "Node 1" // Create variable for Node ID


// I2C Object Creation
RV8803 rtc; // Create an RTC object
Adafruit_SHT4x sht4 = Adafruit_SHT4x(); // Create a SHT45 object
Adafruit_PM25AQI aqi;

// Test counter: Variable
int counter = 0;

// AQI Calculation Function Prototypes
int calculateAQI(float concentration, const int breakpoints[][2], const int AQI_ranges[][2], int size);
std::pair<int, String> calculateDominantAQI(float pm25, float pm10);

void setup() {
  Serial.begin(9600); //  Start Serial Monitor for debugging
  while (!Serial);

  // Initialize GPIO Pins
  Wire.begin();

  Serial.println("LoRa " + String(NODE_ID));

  // Initialize RTC
  if (rtc.begin() == false) {
    Serial.println("RV-8803 not detected. Please check wiring.");
    while (1);
  }

   // Manually set the RTC to the compile time (Comment out when in Demo)
  setRTCToCompileTime();

  // Debugging prints
  Serial.print("Compile Time: ");
  Serial.print(__TIME__);
  Serial.print(" Date: ");
  Serial.println(__DATE__);

  // Print the current time and date
  rtc.updateTime();
  Serial.println("RTC initialized! Current date and time:");
  Serial.print(rtc.stringDateUSA());  // Print date in MM/DD/YYYY format
  Serial.print(" ");
  Serial.println(rtc.stringTime());   // Print time in HH:MM:SS format

  // Initialize SHT45
  if (!sht4.begin()) {
    Serial.println("SHT45 not detected. Check Wiring");
    while (1);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION); // Set precision: HIGH
  Serial.println("SHT45 Initialized!");

  // Initialize PMSA003I
  if (!aqi.begin_I2C()) { // Use I2C instead or UART2
  Serial.println("PMSA003I not detected. Please check wiring.");
  while (1);
  }
  Serial.println("PMSA003I Initialized");

  // Initialize LoRa
  if (!LoRa.begin(923200000)) { //Set frequency for Philippines
    Serial.println("Failed to initialize LoRa Module");
    while (1);
  }
  Serial.println("LoRa initialized! " + String(NODE_ID) + " Ready");
}

void loop() {
  // Get current date and time from RTC
  rtc.updateTime();
  String timestamp = String(rtc.getYear()) + "-" +
                     String(rtc.getMonth()) + "-" +
                     String(rtc.getDate()) + "T" +
                     rtc.stringTime(); // Results in "YYYY-MM-DDTHH:MM:SS"

  Serial.println(timestamp); // RTC Date for debugging

  // Read Temperature and Humidity
  sensors_event_t humidity, temp; // Create temp and humidity objects
  sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data

  float temperature = temp.temperature;
  float relHumidity = humidity.relative_humidity;

  // Read PM2.5 and PM10 Values
  PM25_AQI_Data data;
 float pm25 = 0.0, pm10 = 0.0;

  if (aqi.read(&data)) {
    pm25 = data.pm25_env; // PM2.5 in environmental units
    pm10 = data.pm10_env; // PM10 in environmental units
  } else {
    Serial.println("Failed to read from PM2.5 sensor.");
  }

 // Calculate the dominant AQI
  std::pair<int, String> domAQI = calculateDominantAQI(pm25, pm10);

  // Extract the values from the pair
  int aqi = domAQI.first;
  String domPollutant = domAQI.second;

  // Create JSON data packet
  String dataPacket = "{";
  dataPacket += "\"node\":\"" + String(NODE_ID) + "\",";
  dataPacket += "\"time\":\"" + timestamp + "\",";
  dataPacket += "\"temp\":" + String(temperature, 2) + ",";
  dataPacket += "\"hum\":" + String(relHumidity, 2) + ",";
  dataPacket += "\"pm25\":" + String(pm25, 2) + ",";
  dataPacket += "\"pm10\":" + String(pm10, 2) + ",";
  dataPacket += "\"aqi\":" + String(aqi) + ",";
  dataPacket += "\"pollutant\":\"" + String(domPollutant) + "\",";
  dataPacket += "\"count\":" + String(counter);
  dataPacket += "}";

  // Calculate the Packet Size
  int packetSize = dataPacket.length();
  Serial.print("Packet Size: "); // For debugging
  Serial.print(packetSize);
  Serial.println(" bytes");

  // Check for Packet Size Limit
  if (packetSize > 255) { // Limit typically 256 bytes
    Serial.println("Error: Packet size exceeds LoRa payload limit");
    return; // Skip sending packet
  }

  //Send data packet
  LoRa.beginPacket(); // Starts a new packet
  LoRa.print(dataPacket);
  LoRa.endPacket();

  Serial.println(String(NODE_ID) + " sent: " + dataPacket); // Print send data for debugging

  counter++; // Counter Increment

  delay(60000);
}

// Create Function for RTC-Compile Time Sync
void setRTCToCompileTime() {
  const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  char month[4];
  int day, year, hour, minute, second;
  sscanf(__DATE__, "%s %d %d", month, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
  int monthIndex = 0;
  for (int i = 0; i < 12; i++) {
    if (strcmp(month, monthName[i]) == 0) {
      monthIndex = i + 1;
      break;
    }
  }
  rtc.setTime(second, minute, hour, 0, day, monthIndex, year);

  // Debugging prints
  Serial.print("Compile Time: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
}

// Creat Function to Calculate AQI for a Given Pollutant Concentration
int calculateAQI(float concentration, const int breakpoints[][2], const int AQI_ranges[][2], int size) {
  for (int i = 0; i < size; i++) {
    if (concentration <= breakpoints[i][1]) {
      float AQI = ((float)(AQI_ranges[i][1] - AQI_ranges[i][0]) / (breakpoints[i][1] - breakpoints[i][0]))
                  * (concentration - breakpoints[i][0]) + AQI_ranges[i][0];
      return round(AQI);
    }
  }
  return -1; // Return -1 if concentration is out of range
}

// Create Function to Calculate and Compare AQI for PM2.5 and PM10
std::pair<int, String> calculateDominantAQI(float pm25, float pm10) {
  // Breakpoints for PM2.5 in µg/m³
  const int PM25_breakpoints[][2] = {
    {0, 12}, // Good
    {12, 35}, // Moderate
    {35, 55}, // Unhealthy for Sensitive Groups
    {55, 150}, // Unhealthy
    {150, 250}, // Very Unhealthy
    {250, 350}, // Hazardous
    {350, 500}}; // Run for your life
  const int PM25_AQI_ranges[][2] = {
    {0, 50},
    {51, 100},  
    {101, 150}, 
    {151, 200}, 
    {201, 300}, 
    {301, 400}, 
    {401, 500}};
  int PM25_size = sizeof(PM25_breakpoints) / sizeof(PM25_breakpoints[0]);

  // Breakpoints for PM10 in µg/m³
  const int PM10_breakpoints[][2] = {
    {0, 54}, 
    {55, 154}, 
    {155, 254}, 
    {255, 354}, 
    {355, 424}, 
    {425, 504}, 
    {505, 604}};
  const int PM10_AQI_ranges[][2] = {
    {0, 50}, 
    {51, 100}, 
    {101, 150}, 
    {151, 200}, 
    {201, 300}, 
    {301, 400}, 
    {401, 500}};
  int PM10_size = sizeof(PM10_breakpoints) / sizeof(PM10_breakpoints[0]);

  // Calculate AQI for PM2.5 and PM10
  int AQI_PM25 = calculateAQI(pm25, PM25_breakpoints, PM25_AQI_ranges, PM25_size);
  int AQI_PM10 = calculateAQI(pm10, PM10_breakpoints, PM10_AQI_ranges, PM10_size);

  // Determine the dominant AQI
  String dominantPollutant = "";
  int dominantAQI = 0;
  if (AQI_PM25 > AQI_PM10) {
    dominantAQI = AQI_PM25;
    dominantPollutant = "PM2.5";
  } else {
    dominantAQI = AQI_PM10;
    dominantPollutant = "PM10";
  }

  // Return result as a pair
  return std::make_pair(dominantAQI, dominantPollutant);
}