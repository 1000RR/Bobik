import eventlet
eventlet.monkey_patch()

import subprocess
import threading
import alarm
import json
import os
from queue import Queue
import ssl
from flask import Flask, jsonify, request
from flask_socketio import SocketIO, emit
from uuid import uuid4

thisDir = os.path.dirname(os.path.abspath(__file__))
serverKeysDir = thisDir + "/server-keys"


CERT_EXPIRY_DATE = subprocess.run(
    [serverKeysDir + "/get-cert-expiry.sh"],       # path to your script
    stdout=subprocess.PIPE,  # capture stdout
    stderr=subprocess.PIPE,  # capture stderr (optional)
    text=True                # decode to string (instead of bytes)
).stdout.rstrip("\n")

print("SSL CERT EXPIRY DATE: " + CERT_EXPIRY_DATE)

CORS = ["https://bobik.lan:5020", "https://192.168.2.100", "https://192.168.2.100:443", "https://192.168.2.100:5020", "https://192.168.2.100:5010", "http://192.168.2.100:3000", "https://bobik.lan","https://192.168.99.5"]

# Create a queue for communication between main program and daemon thread
webserver_message_queue = Queue()
responseQueues = {}
# Set up the Flask web API
app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins=CORS, ping_timeout=11, ping_interval=5, async_mode="eventlet")
thread = None
thread_lock = threading.Lock()
new_client_exists = False
alarmQueueMessages = {}
print(thisDir)
client_count = 0

def main():
    global responseQueues
    global webserver_message_queue
    global thisDir

    print("Main program started.")

    # Create a thread for the raspi alarm python script and pass the global variable and message queue
    alarm_thread = threading.Thread(target=alarm.run, args=(webserver_message_queue, ), daemon=True)
    alarm_thread.start()

    @app.route('/status', methods=['GET'])
    def get_status():
        status = {"status": "ok"}
        return jsonify(status)

    @app.after_request
    def add_no_cache_header(response):
        response.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate, max-age=0'
        response.headers['Pragma'] = 'no-cache'
        response.headers['Expires'] = '0'
        return response


    @socketio.on('connect')
    def handle_connect():
        #authenticate()
        global new_client_exists

        new_client_exists = True
        print('Client connected')
        global thread
        with thread_lock:
            if thread is None:
                thread = socketio.start_background_task(update_status_thread)

    @socketio.on('disconnect')
    def disconnect():
        global client_count
        client_count -= 1
        if (client_count == 0):
            thread.kill()
        print('Disconnected')

    @socketio.on('getPastEvents')
    def getPastEvents(message):
        global responseQueues
        ip = request.remote_addr
        callUUID = generateUUID()
        responseQueues[callUUID] = Queue()
        messageToSend = {"request":"GET-PAST-EVENTS", "uuid": callUUID, "responseQueue": responseQueues[callUUID], "ip": ip}
        webserver_message_queue.put(messageToSend)
        response = responseQueues[callUUID].get(True, 5)["response"]
        del responseQueues[callUUID]
        emit('postPastEvents', {'message': json.loads(response)})

    @socketio.on('getStatus')
    def getStatus(message):
        sendAlarmStatus("something")

    @socketio.on('arm')
    def arm(message):
        print('>>>>ARMING')
        ip = request.remote_addr
        callUUID = generateUUID()
        messageToSend = {"request":"ENABLE-ALARM", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('disarm')
    def disarm(message):
        print('>>>DISARMING')
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"DISABLE-ALARM", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('alarmSoundOn')
    def arm(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"FORCE-ALARM-SOUND-ON", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('clearOldData')
    def arm(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"CLEAR-OLD-DATA", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('checkPhones')
    def arm(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"ALERT-CHECK-PHONES", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('toggleGarageDoorState')
    def disarm(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"TOGGLE-GARAGE-DOOR-STATE", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('cansendrepeatedly')
    def cansendrepeatedly(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"CAN-REPEATEDLY-SEND-" + message['message'], "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('cansendsingle')
    def cansendsingle(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"CAN-SINGLE-SEND-" + message['message'], "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('canstopsending')
    def canstopsending(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"CAN-STOP-SENDING", "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on('getAlarmProfiles')
    def getProfiles(message):
        global responseQueues

        callUUID = generateUUID()
        responseQueues[callUUID] = Queue()
        ip = request.remote_addr
        messageToSend = {"request":"GET-ALARM-PROFILES", "uuid": callUUID, "responseQueue": responseQueues[callUUID], "ip": ip}
        webserver_message_queue.put(messageToSend)
        response = responseQueues[callUUID].get(True, 5)["response"]
        del responseQueues[callUUID]
        emit('postAlarmProfiles', {'message': json.loads(response)})

    @socketio.on('setAlarmProfile')
    def setAlarmProfile(message):
        callUUID = generateUUID()
        ip = request.remote_addr
        messageToSend = {"request":"SET-ALARM-PROFILE-" + str(message['message']), "uuid": callUUID, "ip": ip}
        webserver_message_queue.put(messageToSend)

    @socketio.on_error()
    def error(e):
        print('Error', e)
       
    #for sure wth werkzeug
    # sslContext = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    # sslContext.load_cert_chain(certfile=serverKeysDir+'/bobik-cert.pem', keyfile=serverKeysDir+'/bobik-key.pem')

    ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_context.load_cert_chain(certfile=serverKeysDir+'/bobik-cert.pem', keyfile=serverKeysDir+'/bobik-key.pem')

    # Run the Flask app / werkzeug
    #socketio.run(app, host='0.0.0.0', port=8080, allow_unsafe_werkzeug=True)
    #socketio.run(app, host='0.0.0.0', port=8080, ssl_context=sslContext, allow_unsafe_werkzeug=True)
    # Run the Flask app with eventlet
    listener = eventlet.listen(('0.0.0.0', 8080))
    wrapped_socket = ssl_context.wrap_socket(listener, server_side=True)
    eventlet.wsgi.server(wrapped_socket, app)
   

def update_status_thread():
    last_status_str = 0
    while True:
        #print(f"before update last_client_count {last_client_count}")
        last_status_str = sendAlarmStatus(last_status_str)
        #print(f"updated last_client_count {last_client_count}")
        socketio.sleep(1)

def getClientCount():
    global client_count
    #print('RETURNING CLIENT COUNT ' + str(client_count))
    return client_count

def generateUUID():
    return uuid4().hex

def sendAlarmStatus (last_status_str):
    global new_client_exists
    global responseQueues

    callUUID = generateUUID()
    responseQueues[callUUID] = Queue()
    messageToSend = {"request":"ALARM-STATUS", "uuid": callUUID, "responseQueue": responseQueues[callUUID], "ip": "not-parsed-intentionally"}
    webserver_message_queue.put(messageToSend)
   
    # TODO: the following code is for debugging a bug whereby after an Arm&Switch
    # profile button is pressed, no response present on queue after 5 seconds
    # (tried 10, as well), which gets the server into a bad state, with all clients showing "loading"
    #
    # try:
    #     response = responseQueues[callUUID].get(True, 10)["response"]
    # except:
    #     response = last_status_str
    response = responseQueues[callUUID].get(True, 5)["response"]
    del responseQueues[callUUID]
    jsonData = json.loads(response)
    jsonData["sslCertExpiry"] = CERT_EXPIRY_DATE 

    if (response != last_status_str or new_client_exists):
        print("Sending status to connected clients")
        socketio.emit('postStatus', {'message': jsonData})
        new_client_exists = False

    return response
    

if __name__ == '__main__':
    main()
