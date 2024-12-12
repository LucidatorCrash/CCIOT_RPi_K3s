FROM python:3.11

RUN pip install --upgrade pip    


# Install system dependencies for Bluetooth and building Python extensions
#RUN apt-get install -y bluez
RUN pip install git+https://github.com/pybluez/pybluez.git#egg=pybluez
RUN apt-get update && apt-get install -y dbus


# Sets the working directory to /home/pi
WORKDIR /app

# Copies everything from the current directory into the /app folder
COPY . /app

# Installs the dependancies
RUN pip install -r requirements.txt

#RUN pip install --no-cache-dir pybluez

# Make port 8080 available to the world outside this container
EXPOSE 8080

# Prepare dbus socket by mounting from host
#COPY dbus.conf /etc/dbus-1/session.d/
#COPY entrypoint.sh /entrypoint.sh
#RUN chmod +x /entrypoint.sh
#ENTRYPOINT ["dbus-run-session", "python3", "ble_listener.py"]

# Create directory for socket
RUN mkdir -p /var/run/dbus 

# runs the ble_listener.py as soon as the container runs.
CMD ["python", "ble_listener.py"]
#CMD . /opt/venv/bin/activate && exec python ble_listener.py
