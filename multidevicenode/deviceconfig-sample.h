#pragma once

Device devices[] = {
	{
		type : SENSOR,
		myCanId : 0x31, /*garage side door*/
		deviceType : 5,
		ioPin : 5,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this pin (when defined) turns relay ON a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	},
	{
		type : SENSOR,
		myCanId : 0x50, /*front of house door*/
		deviceType : 5,
		ioPin : 6,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this pin (when defined) turns relay ON a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	},
	{
		type : SENSOR,
		myCanId : 0x40, /*back kitchen door*/
		deviceType : 5,
		ioPin : 4,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this pin (when defined) turns relay ON a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	}};

// type: SENSOR and relayPin > -1 means a powered sensor device that can be enabled/disabled
