from flask import Flask, request, jsonify, render_template
import requests

app = Flask(__name__)

# Configured with your explicit ESP32 local IP address and data endpoint
ESP32_DATA_URL = "http://192.168.144.121/data"

@app.route('/')
def index():
    """Serves the frontend dashboard from your templates folder."""
    return render_template('index.html')

@app.route('/data', methods=['GET'])
def get_data():
    """
    Your index.html dashboard requests updates from this route every second.
    Flask requests the real-time sensor metrics directly from your ESP32's 
    local IP, then passes that payload straight to your dashboard.
    """
    try:
        # Fetch the JSON data from your ESP32 server with a strict timeout
        response = requests.get(ESP32_DATA_URL, timeout=1.5)
        
        if response.status_code == 200:
            # Successfully fetched data from ESP32, pass it to the UI
            return jsonify(response.json())
        else:
            return jsonify({"hr": 0, "spo2": 0, "stress": 0, "error": f"ESP32 HTTP Error {response.status_code}"})
            
    except requests.exceptions.RequestException:
        # Handles connection errors cleanly if the ESP32 loses power or connection
        return jsonify({"hr": 0, "spo2": 0, "stress": 0, "error": "Cannot communicate with ESP32"})

if __name__ == '__main__':
    # Flask runs on port 5000. Point your ngrok tunnel here: ngrok http 5000
    app.run(host='0.0.0.0', port=5000, debug=True)