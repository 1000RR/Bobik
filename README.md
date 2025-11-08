
<img width="2696" height="2644" alt="Screenshot 2025-11-08 at 11 54 35 AM" src="https://github.com/user-attachments/assets/f2ec9d8a-f580-4753-894e-d0b82c820c8a" />


## Idea
#### Alarm when certain sensors detect movement and/or entry.
#### Control via WebUI over HTTPS&WSS gated by either mTLS or password for some session length. Observe SSL cert topology best practices. 

### Resilience
#### should be resilient to power outages (for a given amount of time), disconnection of devices on the CAN bus is treated as a security event (alarm trigger) for those devices explicitly being monitored. With the frontend being the main control surface, the topology of network devices' uninterruptable power supply, and possibly WiFi access is critical.


BACKLOG:
- hardware and software support for CANBUS-adjacent Vsource line voltage monitoring and threshold warning in UI / email / alarm(s).
- support for UPS status monitoring and state change warning in UI / email.
- move past events log from memory to DB; DB should have a sweep job based on back date and/or number of events.
  - searching for events from UI.
- front end security: implement CSP.
- install dependencies script should include pip dependencies (both on device at (re)install time and in dev env)
