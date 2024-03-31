import requests

M5StickCPlus_IP_address = '192.168.116.77'  # change to your M5StickCPlus IP address

# Retrieve sensor data
response = requests.get(f'http://{M5StickCPlus_IP_address}/data')
print(response.text)

# Toggle emergency state
response = requests.post(f'http://{M5StickCPlus_IP_address}/emergency')
print(response.text)
