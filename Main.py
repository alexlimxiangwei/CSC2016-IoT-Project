import random
from queue import Queue, Empty
import geocoder
import requests
from flask import Flask, render_template, jsonify, request, Response
import paho.mqtt.client as mqtt


def get_current_location():
    g = geocoder.ip('me')
    if g.ok:
        return g.latlng
    else:
        return None


# Get current latitude and longitude
current_latitude, current_longitude = get_current_location()


def generate_real_data():
    # Generate fake GPS data with small variance
    variance = random.uniform(-0.0001, 0.0001)
    fake_longitude = current_longitude + variance
    fake_latitude = current_latitude + variance
    distance = random.randint(0, 2)  # Distance in meters

    # Generate fake temperature data with bias towards 32 degrees Celsius
    temperature_bias = random.triangular(-5, 5, 0)
    fake_temperature = round(32 + temperature_bias, 1)

    # Generate fake humidity data with bias towards the middle
    humidity_bias = random.triangular(-20, 20, 0)
    fake_humidity = round(50 + humidity_bias, 1)

    return {
        'longitude': fake_longitude,
        'latitude': fake_latitude,
        'distance': distance,
        'temperature': fake_temperature,
        'humidity': fake_humidity
    }


def vary_data(real_data_dict):
    real_data_dict['longitude'] = round(real_data_dict['longitude'] + random.uniform(-0.00001, 0.00001), 5)
    real_data_dict['latitude'] = round(real_data_dict['latitude'] + random.uniform(-0.00001, 0.00001), 5)
    real_data_dict['distance'] = round(real_data_dict['distance'] + random.uniform(0.2, 0.2), 3)
    real_data_dict['temperature'] = round(real_data_dict['temperature'] + random.uniform(-0.1, 0.1), 1)
    real_data_dict['humidity'] = round(real_data_dict['humidity'] + random.uniform(-0.1, 0.1), 1)
    return real_data_dict


real_data = generate_real_data()

# TestStick1_IP_address = '192.168.116.77'  # change to your M5StickCPlus IP address

# MQTT setup
mqtt_broker = "localhost"
mqtt_port = 1883
registration_topic = "m5stick/registration"

registered_devices = []

# Uncomment below for testing:
# stick names cant have spacebar lol
# registered_devices = [('TestStick1', TestStick1_IP_address)]
message_queue = Queue()

try:
    while True:
        message_queue.get_nowait()
except Empty:
    pass


def on_connect(client, userdata, flags, rc, properties):
    print("Connected to MQTT broker")
    client.subscribe(registration_topic)


def on_message(client, userdata, msg):
    if msg.topic == registration_topic:
        device_info = msg.payload.decode().split(",")
        device_name = device_info[0]
        device_ip = device_info[1]
        if (device_name, device_ip) not in registered_devices:
            registered_devices.append((device_name, device_ip))
            print(f"Registered device: {device_name} - IP: {device_ip}")
            message_queue.put(('system', 'new_device_registered', device_name))
            client.subscribe(f"{device_ip}/emergency")
            client.subscribe(f"{device_ip}/fall")
    else:
        print(f"Received message: {msg.topic} - {msg.payload}")
        device_ip = msg.topic.split("/")[0]
        value = msg.payload.decode()
        message_queue.put((device_ip, msg.topic.split("/")[-1], value))


mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# # Retrieve sensor data
# response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
# print(response.text)

# # Toggle emergency state
# response = requests.post(f'http://{M5StickCPlus_IP_address}/emergency')
# print(response.text)

app = Flask(__name__)


@app.route('/')
def index():
    global real_data
    device_data = []
    for device_name, device_ip in registered_devices:
        response = requests.get(f'http://{device_ip}/data')
        data = response.text
        acceleration = data.split('Acceleration: ')[1].split('\n')[0]
        step_count = data.split('Step Count: ')[1].split('\n')[0]
        fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
        emergency = data.split('Emergency: ')[1].strip('\n')[0]
        alert = data.split('Alert: ')[1].strip()
        real_data = vary_data(real_data)
        device_data.append({
            'device_ip': device_ip,
            'device_name': device_name,
            'acceleration': acceleration,
            'step_count': step_count,
            'fall_detected': fall_detected,
            'emergency': emergency,
            'alert': alert,
            'longitude': real_data['longitude'],
            'latitude': real_data['latitude'],
            'distance': real_data['distance'],
            'temperature': real_data['temperature'],
            'humidity': real_data['humidity']
        })
    return render_template('index.html', device_data=device_data)


@app.route('/data')
def get_data():
    global real_data
    device_ip = request.args.get('device_ip')
    if device_ip:
        # Get data for a specific device
        for device_name, registered_device_ip in registered_devices:
            if registered_device_ip == device_ip:
                response = requests.get(f'http://{device_ip}/data')
                data = response.text
                acceleration = data.split('Acceleration: ')[1].split('\n')[0]
                step_count = data.split('Step Count: ')[1].split('\n')[0]
                fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
                emergency = data.split('Emergency: ')[1].strip('\n')[0]
                alert = data.split('Alert: ')[1].strip()
                real_data = vary_data(real_data)
                device_data = {
                    'device_ip': device_ip,
                    'device_name': device_name,
                    'acceleration': acceleration,
                    'step_count': step_count,
                    'fall_detected': fall_detected,
                    'emergency': emergency,
                    'alert': alert,
                    'longitude': real_data['longitude'],
                    'latitude': real_data['latitude'],
                    'distance': real_data['distance'],
                    'temperature': real_data['temperature'],
                    'humidity': real_data['humidity']
                }
                return jsonify(device_data)
        return jsonify({'error': 'Device not found'})
    else:
        # Get data for all devices
        device_data = []
        for device_name, device_ip in registered_devices:
            response = requests.get(f'http://{device_ip}/data')
            data = response.text
            acceleration = data.split('Acceleration: ')[1].split('\n')[0]
            step_count = data.split('Step Count: ')[1].split('\n')[0]
            fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
            emergency = data.split('Emergency: ')[1].strip('\n')[0]
            alert = data.split('Alert: ')[1].strip()
            real_data = vary_data(real_data)
            device_data.append({
                'device_ip': device_ip,
                'device_name': device_name,
                'acceleration': acceleration,
                'step_count': step_count,
                'fall_detected': fall_detected,
                'emergency': emergency,
                'alert': alert,
                'longitude': real_data['longitude'],
                'latitude': real_data['latitude'],
                'distance': real_data['distance'],
                'temperature': real_data['temperature'],
                'humidity': real_data['humidity']
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
    response = requests.post(f'http://{device_ip}/toggle_alert')
    print(response.text)
    return 'OK'


@app.route('/stream')
def stream():
    def event_stream():
        while True:
            try:
                # Try to get a message from the queue without blocking
                device_ip, event, value = message_queue.get_nowait()
                if is_noise((device_ip, event, value)):
                    continue  # Skip this message if it's considered noise
                print(f"Sending event: {device_ip}, {event}, {value}")
                if event == 'new_device_registered':
                    yield f"event: new_device\ndata: {value}\n\n"
                else:
                    yield f"data: {device_ip},{event},{value}\n\n"
            except Empty:
                # The queue is empty; yield a keep-alive comment
                yield ': keep-alive\n\n'
    return Response(event_stream(), mimetype="text/event-stream")

def is_noise(item):
    device_ip, event, value = item
    if event == 'heartbeat':
        return True
    return False


if __name__ == '__main__':
    mqtt_client.connect(mqtt_broker, mqtt_port)
    mqtt_client.loop_start()
    app.run(debug=True)
