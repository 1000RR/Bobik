
<img width="2696" height="2644" alt="Screenshot 2025-11-08 at 11 54 35 AM" src="https://github.com/user-attachments/assets/f2ec9d8a-f580-4753-894e-d0b82c820c8a" />

In action: https://github.com/user-attachments/assets/3d294808-4be6-42b3-b0bd-ddd065aa5b89


## Purpose 
#### Perimeter home/office alarm using a multitude of hardware motion and perimeter breach alarms. Controlled via a web UI on a local IP or domain with mTLS client authentication.

### Resilience
#### Resilient to power outages for a reasonable amount of time
#### Timeout of expected devices on the CAN bus is treated as a logged security event (alarm trigger)
#### In the absence of a hardware arm/disarm trigger (ie code keypad, garage opener trigger, etc) WiFi or other network access means is critical to the operation of the control plane


BACKLOG:
- move past events log from memory to DB. In Control UI, show events in a paginated fashion (boundary of today in this TZ - 7 days; fetch prior button fetches prior 7 days; define maximum number of events in window / reuse DOM nodes so as to not take performance penalty. 
- dockerize python server (controller)
- mTLS client cert revocation (state + nginx checks)
- hardware and software support for CANBUS-adjacent Vsource line voltage monitoring and threshold warning in UI / email / alarm(s).

 
