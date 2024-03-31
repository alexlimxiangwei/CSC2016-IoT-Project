import requests
from flask import Flask, render_template, jsonify, request

M5StickCPlus_IP_address = '192.168.116.77'  # change to your M5StickCPlus IP address

# Retrieve sensor data
response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
print(response.text)

# Toggle emergency state
response = requests.post(f'http://{M5StickCPlus_IP_address}/emergency')
print(response.text)

app = Flask(__name__)

@app.route('/')
def index():
    # Retrieve sensor data from M5StickCPlus
    response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
    data = response.text

    # Parse the sensor data
    acceleration = data.split('Acceleration: ')[1].split('\n')[0]
    step_count = data.split('Step Count: ')[1].split('\n')[0]
    fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
    emergency = data.split('Emergency: ')[1].strip()

    # Render the template with the sensor data
    return render_template('index.html',
                           acceleration=acceleration,
                           step_count=step_count,
                           fall_detected=fall_detected,
                           emergency=emergency)

@app.route('/data')
def get_data():
    # Retrieve sensor data from M5StickCPlus
    response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
    data = response.text

    # Parse the sensor data
    acceleration = data.split('Acceleration: ')[1].split('\n')[0]
    step_count = data.split('Step Count: ')[1].split('\n')[0]
    fall_detected = data.split('Fall Detected: ')[1].split('\n')[0]
    emergency = data.split('Emergency: ')[1].strip()

    # Return the sensor data as JSON
    return jsonify({
        'acceleration': acceleration,
        'step_count': step_count,
        'fall_detected': fall_detected,
        'emergency': emergency
    })

@app.route('/reset', methods=['POST'])
def reset_values():
    reset_type = request.form['type']
    requests.post(f'http://{M5StickCPlus_IP_address}/{reset_type}')  # Reset fall / emergency state

    return 'OK'


if __name__ == '__main__':
    app.run(debug=True)