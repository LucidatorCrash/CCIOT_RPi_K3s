# CCIOT_RPi_K3s

## Details
Using the files directly will run the deployment on k3s and by default, creates 2 replicas of the BLE_Listener and MQTT_Publisher. <br>
This file also assumes you have k3s installed and working without issues. <br>
To install k3s, you can check here for the details: https://docs.k3s.io/installation

## Steps to use this:

### **Step 0** <br>
Clone the repo and port it over to your RPi <br>

### **Step 1. Edit mqtt_publisher.py** <br>
1.1 Change ENDPOINT

### **Step 2. Add all your certificate files** <br>
2.1 After adding you certs <br>
2.2 Run the following code to create the secret <br> 
<code>kubectl create secret generic aws-iot-certs \
  --from-file=device.pem.crt=path/to/device.pem.crt \
  --from-file=private.pem.key=path/to/private.pem.key \
  --from-file=AmazonRootCA1.pem=path/to/AmazonRootCA1.pem</code>

### **Step 3. Edit ble_listener.py** <br>
3.1 Add all the device mac addresses you are looking for and their corresponding characteristic UUID <br>

### Step 4. Run the start-up bash script <br>
<code>sudo bash cciot_startup.sh</code>



