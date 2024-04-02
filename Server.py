from flask import Flask, request, jsonify, render_template
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# In-memory storage for sensor data from multiple devices
devices_data = {}

@app.route('/')
def index():
    # Render the template with the device IDs to initiate polling for data
    return render_template('server_view.html', devices=list(devices_data.keys()))

@app.route('/data/<device_id>', methods=['POST'])
def update_data(device_id):
    # Update data for the specified M5StickC Plus device
    devices_data[device_id] = request.json
    return jsonify({'status': 'success', 'device_id': device_id})

@app.route('/data', methods=['GET'])
def get_data():
    # Return the sensor data for all devices
    return jsonify(devices_data)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
