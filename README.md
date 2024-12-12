# CCIOT_RPi_K3s

## __Steps to use this:__ 

### **1. Edit mqtt_publisher.py** <br>
1.1 Change ENDPOINT

### **2. Add all your certificate files** <br>
2.1 After adding you certs <br>
2.2        kubectl create secret generic aws-iot-certs \
  --from-file=device.pem.crt=path/to/device.pem.crt \
  --from-file=private.pem.key=path/to/private.pem.key \
  --from-file=AmazonRootCA1.pem=path/to/AmazonRootCA1.pem

