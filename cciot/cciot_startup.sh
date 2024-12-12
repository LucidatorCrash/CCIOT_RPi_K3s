#!/bin/bash

sudo kubectl apply -f persistentVolume.yaml

sudo kubectl apply -f persistentVolumeClaim.yaml

cd mqtt_publisher/

docker build -t mqtt-publisher:1.0 .

sudo kubectl apply -f publisher_deployment.yaml

cd ../ble_listener/

docker build --network=host -t ble-listener:1.0 .

sudo kubectl apply -f ble_deployment.yaml

cd ..

echo "Step up done!"

sudo kubectl rollout restart deployments

sudo kubectl get pods
