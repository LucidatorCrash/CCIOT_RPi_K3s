#!/bin/bash

# Check if the D-Bus socket is mounted
if [ ! -S /var/run/dbus/system_bus_socket ]; then
    echo "D-Bus socket not found. Attempting to mount..."
    
    # Try mounting the system D-Bus socket from the host if it is available
    if [ -e /var/run/dbus/system_bus_socket ]; then
        mkdir -p /var/run/dbus
        mount --bind /var/run/dbus/system_bus_socket /var/run/dbus/system_bus_socket
        echo "D-Bus socket mounted successfully."
    else
        echo "D-Bus socket is missing on the host system. Exiting."
        exit 1
    fi
fi

# Run the original command
exec "$@"

