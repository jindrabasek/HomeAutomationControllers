/*
 * DaitsuHandling.cpp
 *
 *  Created on: 22. 6. 2017
 *      Author: jindra
 */

#include "DaitsuHandling.h"

#include <core/MySensorsCore.h>

#include <hal/architecture/MyHwESP8266.h>

#include <IRremoteESP8266.h>  // https://github.com/z3t0/Arduino-IRremote/releases
// OR install IRRemote via "Sketch" -> "Include Library" -> "Manage Labraries..."
// Search for IRRemote b shirif and press the install button

#include <IRsend.h>
#include "core/MyMessage.h"
#include <MideaIR.h>

#include <Arduino.h>

// Operating modes
#define MODE_OFF    0
#define MODE_AUTO   1
//#define MODE_HEAT   2
#define MODE_COOL   3
#define MODE_DRY    4
#define MODE_FAN    5

// Fan speeds. Note that some heatpumps have less than 5 fan speeds
#define FAN_AUTO    0
#define FAN_1       1
#define FAN_2       2
#define FAN_3       3

DaitsuHandling::~DaitsuHandling() {
	// TODO Auto-generated destructor stub
}

void DaitsuHandling::setup() {

	irsend.begin();

	Serial.println(F("Daitsu init done..."));
}

void DaitsuHandling::presentation() {
	// Register a sensors to gw. Use binary light for test purposes.
	present(daitsuControllSensId, S_INFO, "Daitsu Command");
	present(daitsuSendCmdSensId, S_BINARY, "Daitsu Send");
	present(swingSensId, S_BINARY, "Daitsu Swing");
}

DaitsuHandling::DaitsuHandling(uint8_t daitsuControllSensId,
		uint8_t daitsuSendCmdSensId, uint8_t swingSensId, uint16_t IRsendPin) :
		daitsuControllSensId(daitsuControllSensId), daitsuSendCmdSensId(
				daitsuSendCmdSensId), swingSensId(swingSensId), msgDaitsuControll(
				daitsuControllSensId, V_TEXT), msgDaitsuSendCmd(
				daitsuSendCmdSensId, V_STATUS), msgSwing(swingSensId, V_STATUS), irsend(
				IRsendPin) {
}

void DaitsuHandling::receive(const MyMessage &message) {

	if (message.type == V_STATUS && message.sensor == swingSensId) {
		Serial.println(F("Do swing"));
		MideaIR remoteControl(&irsend);
		remoteControl.doOscilate();
	}

	else if (message.type == V_STATUS
			&& message.sensor == daitsuSendCmdSensId) {
		// When the button is pressed on Domoticz, request the value of the TEXT sensor
		Serial.println(F("Requesting Daitsu command from Domoticz..."));
		request(daitsuControllSensId, V_TEXT, 0);
	}

	else if (message.type == V_TEXT && message.sensor == daitsuControllSensId) {

		const char *irData;

		// TEXT sensor value is received as a result of the previous step
		Serial.println(F("Daitsu command received from Domoticz..."));
		Serial.print(F("Code: 0x"));

		irData = message.getString();
		Serial.println(irData);

		long irCommand = 0;
		int sscanfStatus = sscanf(irData, "%lx", &irCommand);

		if (sscanfStatus == 1) {
			MideaIR remoteControl(&irsend);

			Serial.print(F("IR code conversion OK: 0x"));
			Serial.println(irCommand, HEX);

			/*
			 The heatpump command is packed into a 32-bit hex number, see
			 libraries\HeatpumpIR\HeatpumpIR.h for the constants
			 12345678
			 5 OPERATING MODE
			 6 FAN SPEED
			 78 TEMPERATURE IN HEX
			 */

			byte mode = (irCommand & 0x0000F000) >> 12;
			byte fan = (irCommand & 0x00000F00) >> 8;
			byte temp = (irCommand & 0x000000FF);

			remoteControl.setState(true);

			switch (mode) {
				case MODE_OFF:
					remoteControl.setState(false);
					break;
				case MODE_AUTO:
					remoteControl.setMode(mode_auto);
					break;
				case MODE_COOL:
					remoteControl.setMode(mode_cool);
					break;
				case MODE_DRY:
					remoteControl.setMode(mode_no_humidity);
					break;
				case MODE_FAN:
					remoteControl.setMode(mode_ventilate);
					break;
			}

			switch (fan) {
				case FAN_AUTO:
					remoteControl.setSpeedFan(fan_auto);
					break;
				case FAN_1:
					remoteControl.setSpeedFan(fan_speed_1);
					break;
				case FAN_2:
					remoteControl.setSpeedFan(fan_speed_2);
					break;
				case FAN_3:
					remoteControl.setSpeedFan(fan_speed_3);
					break;
			}

			remoteControl.setTemperature(temp);

			Serial.print(F("Daitsu send mode: "));
		    Serial.print(remoteControl.getMode());
		    Serial.print(F(", fan: "));
		    Serial.print(remoteControl.getSpeedFan());
		    Serial.print(F(", temp: "));
		    Serial.print(remoteControl.getTemperature());
		    Serial.print(F(", state: "));
		    Serial.println(remoteControl.getState());

			remoteControl.emit();

		} else {
			Serial.println(F("Failed to convert IR hex code to number"));
		}

		// Set the Domoticz switch back to 'OFF' state
		send(msgDaitsuSendCmd.setSensor(daitsuSendCmdSensId).set(0));
	}

}

