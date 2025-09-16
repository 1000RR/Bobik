#pragma once

Device devices[] = {
	{
		type : SENSOR,
		myCanId : 0x31, /*garage side door*/
		deviceType : 5,
		ioPin : 5,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this turns relay ON on a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	},
	{
		type : SENSOR,
		myCanId : 0x50, /*front of house*/
		deviceType : 5,
		ioPin : 6,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this turns relay ON on a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	},
	{
		type : SENSOR,
		myCanId : 0x40, /*back door kitchen*/
		deviceType : 5,
		ioPin : 4,
		relayPin : -1,	 /* -1 = no relay; writing LOW to this turns relay ON on a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) */
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false
	}};