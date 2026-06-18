#pragma once

Device devices[] = {
	{
		type : ALARM,
		myCanId : 0x50, /*BLARING PIEZO*/
		deviceType : 5,
		ioPin : -1,
		relayPin : 5,	 /* -1 = no relay; writing LOW to this pin (when defined) turns relay ON a sensor */
		sensorVal : LOW, /* variable to store the sensor status (value) - set to HIGH initial for sensors of BOSCH PIR type*/
		isAlarmed : false,
		nextStateChangeTimestamp : 0,
		isEnabled : true, /* false = off; true = on; has direct effect on relay state if relay present */
		buzzerDirection : false,
    	inhibitAlarmSendTimeStop: 0 /*keep at 0. Starts up and will send signal only after this number msec. values from [1, UNSIGNED_LONG_MAX] represent absolute msec timestamps at which it's ok to send ALARM (SENSOR_TRIGGERED) signal */
	}
};
// type: SENSOR and relayPin > -1 means a powered sensor device that can be enabled/disabled
