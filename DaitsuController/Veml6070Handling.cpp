/*
 * Veml6070Handling.cpp
 *
 *  Created on: 27. 6. 2017
 *      Author: jindra
 */

#include "Veml6070Handling.h"

#include <core/MySensorsCore.h>
#include <Wire.h>

const unsigned long SLEEP_TIME = 60000;

float uvIndex;
uint16_t uvIndexValue[12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976,
		1079, 1170 };

// really unusual way of getting data, your read from two different addrs!
#define VEML6070_ADDR_H 0x39
#define VEML6070_ADDR_L 0x38


Veml6070Handling::Veml6070Handling(uint8_t uvSensorId, uint8_t uviSensorId) :
		Task(SLEEP_TIME * 1000), uvSensorId(uvSensorId) , uviSensorId(uviSensorId),
		msgUv(uvSensorId, V_LEVEL), msgUvi(uviSensorId, V_UV) {
}

Veml6070Handling::~Veml6070Handling() {
	// TODO Auto-generated destructor stub
}

void Veml6070Handling::setup() {
	Wire.begin();
	Wire.beginTransmission(VEML6070_ADDR_L);
	Wire.write((1 << 2) | 0x02);
	Wire.endTransmission();
	delay(500);
}

void Veml6070Handling::presentation() {
	present(uvSensorId, S_LIGHT_LEVEL);
	present(uviSensorId, S_UV);
}

void Veml6070Handling::run() {

	if (Wire.requestFrom(VEML6070_ADDR_H, 1) != 1) return;
	uint16_t uv = Wire.read();
	uv <<= 8;
	if (Wire.requestFrom(VEML6070_ADDR_L, 1) != 1) return;
	uv |= Wire.read();

	Serial.print("UV Analog reading: ");
	Serial.println(uv);

	int i;
	for (i = 0; i < 12; i++) {
		if (uv <= uvIndexValue[i]) {
			uvIndex = i;
			break;
		}
	}

	//calculate 1 decimal if possible
	if (i > 0) {
		float vRange = uvIndexValue[i] - uvIndexValue[i - 1];
		float vCalc = uv - uvIndexValue[i - 1];
		uvIndex += (1.0 / vRange) * vCalc - 1.0;
	}

	Serial.print("UVI: ");
	Serial.println(uvIndex,2);

	send(msgUv.set(uv));

	send(msgUvi.set(uvIndex, 2));

}
