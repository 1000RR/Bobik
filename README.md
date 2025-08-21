
<img width="899" alt="bobik" src="https://github.com/user-attachments/assets/b32ab92e-6264-4e49-999e-a96790421e8a" />


## Idea
#### Alarm when certain sensors detect movement and/or entry.
#### Control via WebUI over HTTPS&WSS gated by either mTLS or password for some session length. Observe SSL cert topology best practices. 

### Resilience
#### should be resilient to power outages (for a given amount of time), disconnection of devices on the CAN bus is treated as a security event (alarm trigger) for those devices explicitly being monitored. With the frontend being the main control surface, the topology of network devices' uninterruptable power supply, and possibly WiFi access is critical.


BACKLOG:
- hardware and software support for CANBUS-adjacent Vsource line voltage monitoring and threshold warning in UI / email / alarm(s).
- evaluate whether the web server and the arduino-facing alarm.py should run separately and use IPC. Hunch: GIL's lack of concurrency between 2 threads (server, alarm.py - polling) is crappy and will thrash with still a reasonable number of clients.
- support for UPS status monitoring and state change warning in UI / email.
- more past events log from memory to DB; DB should have a sweep job based on back date and/or number of events.
  - searching for events from UI.
- minimize hard coding. Load as much as possible from config files and DB.
- move access password from nginx.conf.
- control unit time and/or client time displayed in UI in local/GMT formats.
- front end security: implement CSP.
- install dependencies script should include pip dependencies (both on device at (re)install time and in dev env)
