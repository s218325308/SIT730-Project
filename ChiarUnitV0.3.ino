#include <ArduinoBLE.h>
#include <DHT.h>

#define DHTPIN 2          // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11     // DHT 11
#define PRESSURE_PIN A0   // Analog pin connected to the pressure sensor
#define MASTER_BUTTON_PIN 3 // Digital pin connected to the master button

bool childInSeat = false; // Default status
bool masterButtonState = false; // Default master button state

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

BLEService customService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic tempCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLECharacteristic seatStatusCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLECharacteristic pingCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite, 20);

unsigned long previousMillis = 0;
const long interval = 30000; // Interval in milliseconds (30 seconds) to send the message

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("\n---Starting---");

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  Serial.print("My BLE MAC:\t\t ");
  Serial.println(BLE.address());
  Serial.println();

  // Set local name and advertising service
  BLE.setLocalName("ChildSeatUnit-01");
  BLE.setAdvertisedService(customService);

  // Add characteristics to the service
  customService.addCharacteristic(tempCharacteristic);
  customService.addCharacteristic(seatStatusCharacteristic);
  customService.addCharacteristic(pingCharacteristic);

  // Add service
  BLE.addService(customService);

  // Start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

  // Initialize DHT sensor
  dht.begin();

  // Initialize master button pin
  pinMode(MASTER_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Check if a central device is connected
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    // Loop while the central is connected
    while (central.connected()) {
      // Check if data is available on the ping characteristic
      if (pingCharacteristic.written()) {
        // Read the value written to the ping characteristic
        int pingData = pingCharacteristic.value()[0];

        // Check if the ping signal is received
        if (pingData == 1) {
          // Respond with the latest sensor values
          sendSensorValues();
        }
      }

      // Poll for BLE events
      BLE.poll();
    }
  }

  // Check the state of the master button
  if (digitalRead(MASTER_BUTTON_PIN) == LOW) { // Button is pressed when the pin reads LOW
    Serial.println("Master button pressed!");
    // Toggle masterButtonState
    masterButtonState = !masterButtonState;
    Serial.println(masterButtonState);
    delay(200); // Debounce delay
  }

  // Update childInSeat status based on masterButtonState or pressure sensor reading
  if (masterButtonState) {
    childInSeat = true; // Override childInSeat to true if masterButtonState is true
  } else {
    int pressureValue = analogRead(PRESSURE_PIN);
    updateChildInSeatStatus(pressureValue);
  }
}

void sendSensorValues() {
  // Read temperature from DHT sensor
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return;
  }

  // Send temperature
  String tempMessage = String(temperature);
  tempCharacteristic.writeValue(tempMessage.c_str(), tempMessage.length());
  Serial.print("Temperature sent: ");
  Serial.println(tempMessage);

  // Send child-in-seat status
  String seatStatusMessage = String(childInSeat ? "True" : "False");
  seatStatusCharacteristic.writeValue(seatStatusMessage.c_str(), seatStatusMessage.length());
  Serial.print("Child-in-seat status sent: ");
  Serial.println(seatStatusMessage);
}

void updateChildInSeatStatus(int pressureValue) {
  // Update child in seat status based on pressure reading
  if (pressureValue > 10) {
    childInSeat = true;
  } else {
    childInSeat = false;
  }
}
