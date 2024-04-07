import requests
from flask import Flask, render_template, jsonify, request
import paho.mqtt.client as mqtt

TestStick1_IP_address = '192.168.116.77'  # change to your M5StickCPlus IP address

# MQTT setup
mqtt_broker = "localhost"
mqtt_port = 1883
registration_topic = "m5stick/registration"

registered_devices = []

# Uncomment below for testing:
# stick names cant have spacebar lol
# registered_devices = [('TestStick1', TestStick1_IP_address)]


def on_connect(client, userdata, flags, rc, properties):
    print("Connected to MQTT broker")
    client.subscribe(registration_topic)


def on_message(client, userdata, msg):
    device_info = msg.payload.decode().split(",")
    device_name = device_info[0]
    device_ip = device_info[1]
    registered_devices.append((device_name, device_ip))
    print(f"Registered device: {device_name} - IP: {device_ip}")


mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# # Retrieve sensor data
# response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
# print(response.text)
#
# # Toggle emergency state
# response = requests.post(f'http://{M5StickCPlus_IP_address}/emergency')
# print(response.text)

app = Flask(__name__)


@app.route('/')
def index():
    device_data = []
    for device_name, device_ip in registered_devices:
        response = requests.get(f'http://{device_ip}/data')
        data = response.text
        acceleration = data.split('Acceleration: ')[1].split('\n')[0]
        step_count = data.split('Step Count: ')[1].split('\n')[0]
        fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
        emergency = data.split('Emergency: ')[1].strip()
        device_data.append({
            'device_ip': device_ip,
            'device_name': device_name,
            'acceleration': acceleration,
            'step_count': step_count,
            'fall_detected': fall_detected,
            'emergency': emergency
        })
    return render_template('index.html', device_data=device_data)


@app.route('/data')
def get_data():
    device_data = []
    for device_name, device_ip in registered_devices:
        response = requests.get(f'http://{device_ip}/data')
        data = response.text
        acceleration = data.split('Acceleration: ')[1].split('\n')[0]
        step_count = data.split('Step Count: ')[1].split('\n')[0]
        fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
        emergency = data.split('Emergency: ')[1].strip()
        device_data.append({
            'device_ip': device_ip,
            'device_name': device_name,
            'acceleration': acceleration,
            'step_count': step_count,
            'fall_detected': fall_detected,
            'emergency': emergency
        })
    return jsonify(device_data)


@app.route('/reset', methods=['POST'])
def reset_values():
    reset_type = request.form['type']
    device_ip = request.form['device_ip']
    requests.post(f'http://{device_ip}/{reset_type}')
    return 'OK'


@app.route('/toggle_alert', methods=['POST'])
def toggle_alert():
    device_ip = request.form['device_ip']
    print(device_ip)

    response = requests.post(f'http://{device_ip}/toggle_alert')
    print(response.text)
    return 'OK'


if __name__ == '__main__':
    mqtt_client.connect(mqtt_broker, mqtt_port)
    mqtt_client.loop_start()
    app.run(debug=True)
