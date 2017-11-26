/*
 * Bme280Handling.cpp
 *
 *  Created on: 23. 6. 2017
 *      Author: jindra
 */

#include "Bme280Handling.h"

#include <core/MySensorsCore.h>

// ----------------------------------------------------------------------------
// I2C setup: SDA and SCL pin selection
// ----------------------------------------------------------------------------
#include <Wire.h>

// If using the standard MySensor wiring direction we can't use default I2C ports. I2C ports moved next to the second GND and 3.3v ports for convenient wiring of I2c modules.
//#define SDA 3  //default to GPIO 4 (D2) but NRF24L01+ CE is plugged on D2 by default for the ESP gateway. SDA changed to GPIO 3 (RX)
//#define SCL 1 //default to GPIO 5 (D1). SDA changed to GPIO 1 (TX)

// ----------------------------------------------------------------------------
// BME280 libraries and variables
// Written by Limor Fried & Kevin Townsend for Adafruit Industries.
// https://github.com/adafruit/Adafruit_BME280_Library
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ----------------------------------------------------------------------------
// Weather station variables and functions
// ----------------------------------------------------------------------------
// Adapt this constant: set it to the altitude above sealevel at your home location.
const float ALTITUDE = 327.1; // meters above sea level. Use your smartphone GPS to get an accurate value!

// Set this to true if you want to send values altough the values did not change.
// This is only recommended when not running on batteries.
const bool SEND_ALWAYS = true;

// Constant for the world wide average pressure
const float SEALEVEL_PRESSURE = 1013.25;

// Sleep time between reads (in ms). Do not change this value as the forecast algorithm needs a sample every minute.
const unsigned long SLEEP_TIME = 60000;

const char *weatherStrings[] = { "stable", "sunny", "cloudy", "unstable",
		"thunderstorm", "unknown" };
enum FORECAST {
	STABLE = 0,       // Stable weather
	SUNNY = 1,        // Slowly rising HP stable good weather
	CLOUDY = 2,      // Slowly falling Low Pressure System, stable rainy weather
	UNSTABLE = 3,     // Quickly rising HP, not stable weather
	THUNDERSTORM = 4, // Quickly falling LP, Thunderstorm, not stable
	UNKNOWN = 5       // Unknown, more time needed
};

const char *situationStrings[] = { "very low", "low", "normal", "high",
		"very high" };
enum WEATHER_SITUATION {
	VERY_LOW_PRESSURE = 0,   // p > -7.5hPa
	LOW_PRESSURE = 1,        // p > -2.5hPa
	NORMAL_PRESSURE = 2,     // p < +/-2.5hPa
	HIGH_PRESSURE = 3,       // p > +2.5hPa
	VERY_HIGH_PRESSURE = 4,  // p > +7.5hPa
};

// this CONVERSION_FACTOR is used to convert from Pa to kPa in the forecast algorithm
// get kPa/h by dividing hPa by 10
#define CONVERSION_FACTOR (1.0/10.0)

Bme280Handling::Bme280Handling(uint8_t tempSensorId, uint8_t humSensorId,
		uint8_t baroSensorId) :
		Task(SLEEP_TIME * 1000), tempSensorId(tempSensorId), humSensorId(
				humSensorId), baroSensorId(baroSensorId), lastSituation(
				NORMAL_PRESSURE), lastPressure(-1), lastTemp(-1), lastHum(-1), tempMsg(
				tempSensorId, V_TEMP), humMsg(humSensorId, V_HUM), pressureMsg(
				baroSensorId, V_PRESSURE), forecastMsg(baroSensorId,
				V_FORECAST), situationMsg(baroSensorId, V_VAR1), forecastMsg2(
				baroSensorId, V_VAR2) {

}

Bme280Handling::~Bme280Handling() {
	// TODO Auto-generated destructor stub
}

void Bme280Handling::setup() {
#if defined(SDA) && defined(SCL)
	Serial.print(F("Using custom I2C pins: SDA on GPIO"));
	Serial.print(SDA);
	Serial.print(F(" and SCL on GPIO"));
	Serial.println(SCL);
	delay(1000);
	Wire.begin(SDA, SCL);
#else
	Serial.println(F("Using default I2C pins: SDA on GPIO4 and SCL on GPIO5"));
	Wire.begin();
#endif
	if (!bme.begin(0x76)) {
		Serial.println(
				F(
						"Could not find a valid BME280 sensor, check wiring or I2C address!"));
		while (1) {
			yield();
		}
	}

	metric = getControllerConfig().isMetric; // was getConfig().isMetric; before MySensors v2.1.1

}

void Bme280Handling::run() {
	// Send locally attached sensor data here
	updatePressureSensor();
}

void Bme280Handling::presentation() {

	// Register sensors to gw (they will be created as child devices)
	present(baroSensorId, S_BARO);
	present(tempSensorId, S_TEMP);
	present(humSensorId, S_HUM);
}

int Bme280Handling::getWeatherSituation(float pressure) {
	int situation = NORMAL_PRESSURE;

	float delta = pressure - SEALEVEL_PRESSURE;
	if (delta > 7.5) {
		situation = VERY_HIGH_PRESSURE;
	} else if (delta > 2.5) {
		situation = HIGH_PRESSURE;
	} else if (delta < -7.5) {
		situation = VERY_LOW_PRESSURE;
	} else if (delta < -2.5) {
		situation = LOW_PRESSURE;
	} else {
		situation = NORMAL_PRESSURE;
	}

	return situation;
}

float Bme280Handling::getLastPressureSamplesAverage() {
	float lastPressureSamplesAverage = 0;
	for (int i = 0; i < LAST_SAMPLES_COUNT; i++) {
		lastPressureSamplesAverage += lastPressureSamples[i];
	}

	lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

	// Uncomment when debugging
	Serial.print(F("### 5min-Average:"));
	Serial.print(lastPressureSamplesAverage);
	Serial.println(F(" hPa"));

	return lastPressureSamplesAverage;
}

// Algorithm found here
// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
// Pressure in hPa -->  forecast done by calculating kPa/h
int Bme280Handling::sample(float pressure) {
	// Calculate the average of the last n minutes.
	int index = minuteCount % LAST_SAMPLES_COUNT;
	lastPressureSamples[index] = pressure;

	minuteCount++;
	if (minuteCount > 185) {
		minuteCount = 6;
	}

	if (minuteCount == 5) {
		pressureAvg = getLastPressureSamplesAverage();
	} else if (minuteCount == 35) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change * 2; // note this is for t = 0.5hour
		} else {
			dP_dt = change / 1.5; // divide by 1.5 as this is the difference in time from 0 value.
		}
	} else if (minuteCount == 65) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) //first time initial 3 hour
		{
			dP_dt = change; //note this is for t = 1 hour
		} else {
			dP_dt = change / 2; //divide by 2 as this is the difference in time from 0 value
		}
	} else if (minuteCount == 95) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 1.5; // note this is for t = 1.5 hour
		} else {
			dP_dt = change / 2.5; // divide by 2.5 as this is the difference in time from 0 value
		}
	} else if (minuteCount == 125) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		pressureAvg2 = lastPressureAvg; // store for later use.
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2; // note this is for t = 2 hour
		} else {
			dP_dt = change / 3; // divide by 3 as this is the difference in time from 0 value
		}
	} else if (minuteCount == 155) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2.5; // note this is for t = 2.5 hour
		} else {
			dP_dt = change / 3.5; // divide by 3.5 as this is the difference in time from 0 value
		}
	} else if (minuteCount == 185) {
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 3; // note this is for t = 3 hour
		} else {
			dP_dt = change / 4; // divide by 4 as this is the difference in time from 0 value
		}
		pressureAvg = pressureAvg2; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
		firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
	}

	int forecast = UNKNOWN;
	if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
			{
		forecast = UNKNOWN;
	} else if (dP_dt < (-0.25)) {
		forecast = THUNDERSTORM;
	} else if (dP_dt > 0.25) {
		forecast = UNSTABLE;
	} else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05))) {
		forecast = CLOUDY;
	} else if ((dP_dt > 0.05) && (dP_dt < 0.25)) {
		forecast = SUNNY;
	} else if ((dP_dt > (-0.05)) && (dP_dt < 0.05)) {
		forecast = STABLE;
	} else {
		forecast = UNKNOWN;
	}

	// Uncomment when dubugging
	Serial.print(F("Forecast at minute "));
	Serial.print(minuteCount);
	Serial.print(F(" dP/dt = "));
	Serial.print(dP_dt);
	Serial.print(F("kPa/h --> "));
	Serial.println(weatherStrings[forecast]);

	return forecast;
}

bool Bme280Handling::updatePressureSensor() {
	bool changed = false;

	float temperature = bme.readTemperature();
	float humidity = bme.readHumidity();
	float pressure_local = bme.readPressure() / 100.0; // Read atmospheric pressure at local altitude
	float pressure = (pressure_local / pow((1.0 - (ALTITUDE / 44330.0)), 5.255)); // Local pressure adjusted to sea level pressure using user altitude

	if (!metric) {
		// Convert to fahrenheit
		temperature = temperature * 9.0 / 5.0 + 32.0;
	}

	int forecast = sample(pressure);
	int situation = getWeatherSituation(pressure);

	if (SEND_ALWAYS || (temperature != lastTemp)) {
		changed = true;
		lastTemp = temperature;
		Serial.print(F("Temperature = "));
		Serial.print(temperature);
		Serial.println(metric ? F(" *C") : F(" *F"));
		if (!send(tempMsg.set(lastTemp, 1))) {
			lastTemp = -1.0;
		}
	}

	if (SEND_ALWAYS || (humidity != lastHum)) {
		lastHum = humidity;
		changed = true;
		Serial.print(F("Humidity = "));
		Serial.print(humidity);
		Serial.println(F(" %"));
		if (!send(humMsg.set(lastHum, 1))) {
			lastHum = -1.0;
		}
	}

	if (SEND_ALWAYS || (pressure != lastPressure)) {
		changed = true;
		lastPressure = pressure;
		Serial.print(F("Sea level Pressure = "));
		Serial.print(pressure);
		Serial.println(F(" hPa"));
		if (!send(pressureMsg.set(lastPressure, 1))) {
			lastPressure = -1.0;
		}
	}

	if (SEND_ALWAYS || (forecast != lastForecast)) {
		changed = true;
		lastForecast = forecast;
		Serial.print(F("Forecast = "));
		Serial.println(weatherStrings[forecast]);
		if (send(forecastMsg.set(weatherStrings[lastForecast]))) {
			if (!send(forecastMsg2.set(lastForecast))) {
			}
		} else {
			lastForecast = -1.0;
		}
	}

	if (SEND_ALWAYS || (situation != lastSituation)) {
		changed = true;
		lastSituation = situation;
		Serial.print(F("Situation = "));
		Serial.println(situationStrings[situation]);
		if (!send(situationMsg.set(lastSituation))) {
			lastSituation = -1.0;
		}
	}

	return changed;
}
