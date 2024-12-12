from bleak import BleakScanner, BleakClient
from datetime import datetime
import time

# Devices to look for
device_to_lookfor = ["CC:DB:A7:2F:F4:52"]
uuid_to_lookfor = [["00dc83ab-1db8-4f44-a78d-cf754866f003"]]

location_map = {
"CC:DB:A7:2F:F4:52": '{"Level": "1", "Unit": "1", "ID": "13"}'
}

# Shared-volume path
sharedVolumePath = "/data/shared/sensor_data.txt"

# Function to read data from a specific GATT characteristic
async def read_write_data(address,uuid, volume_path,map):
    async with BleakClient(address) as client:
        # Replace the characteristic UUID with the correct one
        value = await client.read_gatt_char(uuid)
        decoded_value = value.decode()
        print(value)
        print(f"Data from {address}: {decoded_value}")
        timenow = str(datetime.now())
        data = '{"Body": {"Location":' + map[address] + '}, ' + decoded_value + ', "Timestamp": "' + timenow + '"}'
        with open(volume_path, 'a') as f:
            f.write(data)	# Assuming that the data will write as string to the txt file.

def write_data_simulated(volume_path):
    data = '{Body: {Location: {Level: 1, Unit: 0, ID: 1}, Pressure: 25, Timestamp: 12:50pm}}'
    with open(volume_path, 'a') as f:
        f.write(data)

# async def get_uuid(address):
#     async with BleakClient(address) as client:
#         services = client.services
#         for service in services:
#             service_val = await client.read_gatt_char(service.uuid)
#             print("Got service value")
#             print(f"Service UUID: {service.uuid}\nHas Data: {service_val.decode()}")
#             for characteristic in service.characteristics:
#                 char_val = await client.read_gatt_char(characteristic.uuid)
#                 print("Got characteristic value")
#                 print(f"  Characteristic UUID: {characteristic.uuid}\nHas Data: {char_val.decode()}")

async def main():
    while True:
        devices = await BleakScanner.discover()
        for device in devices:
            print(device)
            if device.address in device_to_lookfor:
                print(device)
                idx = device_to_lookfor.index(device.address)
                for i in range(len(uuid_to_lookfor[idx])):
                    try:
                        print("trying to read")
                        uuid = uuid_to_lookfor[idx][i]
                        await read_write_data(device.address, uuid, sharedVolumePath, location_map)
                        # write_data_simulated(sharedVolumePath)
                    except Exception as e:
                        print(f"Failed to read from {device.address}: {e}")
        time.sleep(5)

import asyncio
asyncio.run(main())
