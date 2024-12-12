From python:3.10-slim

# Sets the working directory to /app
WORKDIR /app

# Copies everything from the current directory into the /app folder
COPY . /app

# Install the requirements.
RUN pip install -r requirements.txt

COPY /AWS_Certificates/everything_CCIOT /app/certs

CMD ["python", "mqtt_publisher.py"]
