import asyncio
from bleak import BleakScanner, BleakClient

MAC = "EC:62:60:8E:26:56"
SEAT_STATUS_CHARACTERISTIC_UUID = "19B10002-E8F2-537E-4F6C-D104768A1214"
PING_CHARACTERISTIC_UUID = "19B10004-E8F2-537E-4F6C-D104768A1214"

async def connect_to_peripheral(timeout=10):
    sensor_data = None

    #print("Scanning for BLE devices...")
    scanner = BleakScanner()
    devices = await scanner.discover()
    peripheral_found = False

    for device in devices:
        if device.address == MAC:
            peripheral_found = True
            #print("Peripheral device found:", device.name, "-", device.address)
            break

    if not peripheral_found:
        print("Peripheral device not found.")
        return None

    async with BleakClient(MAC, timeout=timeout) as client:
        #print("Connected to the peripheral device.")
        sensor_data = await request_sensor_values(client)

    return sensor_data

async def request_sensor_values(client, timeout=10):
    sensor_data = None

    if client.is_connected:
        try:
            # Send a "ping" signal to the Arduino to request sensor values
            await asyncio.wait_for(client.write_gatt_char(PING_CHARACTERISTIC_UUID, bytearray(b"\x01")), timeout)
            #print("Sent ping signal to Arduino.")

            seat = await asyncio.wait_for(client.read_gatt_char(SEAT_STATUS_CHARACTERISTIC_UUID), timeout)
            seat = seat.decode("utf-8").strip()
            #print("Received seat status data:", seat)

            # Structure sensor data into a dictionary
            #sensor_data = {"seat_status": seat}
            sensor_data = f"{seat}"
        except asyncio.TimeoutError:
            print("Operation timed out.")
    else:
        print("Not connected to the peripheral device.")
    
    #print("Received sensor_data:", sensor_data)
    print(sensor_data)
    #return sensor_data

# Run the script if executed directly
if __name__ == "__main__":
    asyncio.run(connect_to_peripheral())

