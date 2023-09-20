from flask import Flask, jsonify, render_template
import os

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/relaunch_test')
def relaunch_test():
    relaunch_sensor_test()
    return jsonify(status="success")

@app.route('/api/data')
def get_data():
    data = read_sensor_data()
    return jsonify(distance=data)

def relaunch_sensor_test():
    os.system("cat /dev/driver")

def read_sensor_data():
    with open('/home/udooer/Projet_kernel/resultat.txt', 'r') as f:
        data = f.read()
    return data

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8888, debug=True)
