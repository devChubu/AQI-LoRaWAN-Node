import serial
import json
import time

# Configure the serial port
ser = serial.Serial("COM10", 9600) # Port for gateway MKRWAN 1310 

# Open the JSON file for writing
with open("data.json", "a") as file:
    while True:
        # Read incoming data from the serial port
        line = ser.readline().decode('utf-8').strip()

        # Check if the line is a valid JSON string
        if line.startswith('{') and line.endswith('}'):
          # Parse the JSON data
          data = json.loads(line)

          # Print the data for debugging
          print("Data saved: ", data)

    time.sleep(1)
