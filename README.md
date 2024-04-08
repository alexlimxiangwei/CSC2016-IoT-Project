# CSC2016-IoT-Project
Team 26
pre installation:
pip install -r requirements.txt

1. start MQTT broker:
cd to the directory where mosquitto.conf is located
mosquitto -v -c mosquitto.conf

2. start the web server:
python Main.py

3. Make sure your web server, broker and M5StickCplus connects to the same network (edit the IP address in the code if necessary)
4. Make sure every M5Stick has a unique name. (Edit in the code)
5. Compile and install project/project.ino onto M5StickCPlus. 
6. Go to http://127.0.0.1:5000 on your browser to view the web page.