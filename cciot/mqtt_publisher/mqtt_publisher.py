import json
import time as t 
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder
ENDPOINT = "xxxxxxxxxxxxxx-xxx.iot.ap-southeast-1.amazonaws.com"
CLIENT_ID = "testdevice"
PATH_TO_CERTIFICATE = "/app/certs/certificate.pem.crt"
PATH_TO_PRIVATE_KEY = "/app/certs/private.pem.key"
PATH_TO_CA1 = "/app/certs/AmazonRootCA1.pem"
# MESSAGE = {"Sensor": 3,"Body": {"Location": {"Level": 1,"Unit": 0,"ID": 1},"Pressure": 25,"Timestamp": "12:50pm"}}
TOPIC = "sensor/data"
RANGE = 1
sharedVolumePath = "/data/shared/sensor_data.txt"

print(f"Using cert: {PATH_TO_CERTIFICATE}")
print(f"Using private key: {PATH_TO_PRIVATE_KEY}")

def collect_data_and_publish(volumePath, topic, client):
    all_text = []
    with open(volumePath, "r+") as f:
        all_text = f.readlines()
    # Add logic for publishing to the MQTT client
    if len(all_text) != 0:
        with open(volumePath,"w") as file:
            pass
        for  i in range(len(all_text)):
            client.publish(topic=topic,payload = json.dumps(all_text[i]),qos=mqtt.QoS.AT_LEAST_ONCE)
    return

# Spin up resources
event_loop_group = io.EventLoopGroup(1)
host_resolver = io.DefaultHostResolver(event_loop_group)
client_bootstrap = io.ClientBootstrap(event_loop_group, host_resolver)
mqtt_connection = mqtt_connection_builder.mtls_from_path(
            endpoint=ENDPOINT,
            cert_filepath=PATH_TO_CERTIFICATE,
            pri_key_filepath=PATH_TO_PRIVATE_KEY,
            client_bootstrap=client_bootstrap,
            ca_filepath=PATH_TO_CA1,
            client_id=CLIENT_ID,
            clean_session=False,
            keep_alive_secs=6
            )

print("Connecting to {} with client ID '{}'...".format(
        ENDPOINT, CLIENT_ID))
# Make the connect() call
connect_future = mqtt_connection.connect()
# Future.result() waits until a result is available
connect_future.result()
print("Connected!")
# Publish message to server desired number of times.
print('Begin Publish')
while True:
    collect_data_and_publish(sharedVolumePath,TOPIC, mqtt_connection)
    t.sleep(3)
print('Publish End')
disconnect_future = mqtt_connection.disconnect()
disconnect_future.result()
