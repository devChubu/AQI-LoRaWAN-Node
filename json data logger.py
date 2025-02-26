import serial
import json
import time

# Configure the serial port
try:
    ser = serial.Serial("COM10", 9600)  # Port for gateway MKRWAN 1310
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)

# Open the JSON file for writing
with open("data.json", "a") as file:
    while True:
        try:
            # Read incoming data from the serial port
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            # Check if the line is a valid JSON string
            if line.startswith('{') and line.endswith('}'):
                try:
                    # Parse the JSON data
                    data = json.loads(line)

                    # Append the data to the JSON file
                    json.dump(data, file)
                    file.write("\n")

                    # Print the data for debugging
                    print("Data saved: ", data)
                except json.JSONDecodeError as e:
                    print(f"Error decoding JSON: {e}")

        except serial.SerialException as e:
            print(f"Error reading from serial port: {e}")

        time.sleep(1)