#include <ArduinoBLE.h>

#define BUTTON_PIN 3      // Digital pin connected to the hardware button

bool driverInSeat = false; // Default status

BLEService customService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic seatStatusCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLECharacteristic pingCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite, 20);

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
  BLE.setLocalName("DVRSeatUnit");
  BLE.setAdvertisedService(customService);

  // Add characteristics to the service
  customService.addCharacteristic(seatStatusCharacteristic);
  customService.addCharacteristic(pingCharacteristic);

  // Add service
  BLE.addService(customService);

  // Start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
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
          sendSeatStatus();
        }
      }

      // Poll for BLE events
      BLE.poll();
    }
  }

  // Check the button state outside of the central connection loop
  if (digitalRead(BUTTON_PIN) == LOW) { // Button is pressed when the pin reads LOW
    Serial.println("Button pressed!");
    // Toggle driverInSeat status when button is pressed
    driverInSeat = !driverInSeat;
    Serial.print("driverInSeat: ");
    Serial.println(driverInSeat);
    delay(200); // Debounce delay
    // sendSeatStatus(); // Send the updated seat status immediately after toggling
  }
}

void sendSeatStatus() {
  // Send driver-in-seat status
  String seatStatusMessage = String(driverInSeat ? "True" : "False");
  seatStatusCharacteristic.writeValue(seatStatusMessage.c_str(), seatStatusMessage.length());
  Serial.print("Driver-in-seat status sent: ");
  Serial.println(seatStatusMessage);
}
