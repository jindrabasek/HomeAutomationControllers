/*
 * Bme280Handling.h
 *
 *  Created on: 23. 6. 2017
 *      Author: jindra
 */

#ifndef BME280HANDLING_H_
#define BME280HANDLING_H_

#include <Task.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <core/MyMessage.h>

class Bme280Handling: public Task {
private:
	uint8_t tempSensorId;
	uint8_t humSensorId;
	uint8_t baroSensorId;

	Adafruit_BME280 bme; // Use I2C
	unsigned long previousMillis = 0;
	int lastSituation;
	float lastPressure;
	float lastTemp;
	float lastHum;
	int lastForecast;

	static const int LAST_SAMPLES_COUNT = 5;
	float lastPressureSamples[LAST_SAMPLES_COUNT];

	int minuteCount = 0;
	bool firstRound = true;
	// average value is used in forecast algorithm.
	float pressureAvg;
	// average after 2 hours is used as reference value for the next iteration.
	float pressureAvg2;

	float dP_dt;
	boolean metric;
	MyMessage tempMsg;
	MyMessage humMsg;
	MyMessage pressureMsg;
	MyMessage forecastMsg;
	MyMessage situationMsg;
	MyMessage forecastMsg2;
public:
	Bme280Handling(uint8_t tempSensorId, uint8_t humSensorId, uint8_t baroSensorId);
	virtual ~Bme280Handling();


	void setup();

	void presentation();

	virtual void run();

private:
	int getWeatherSituation(float pressure);

	float getLastPressureSamplesAverage();

	int sample(float pressure);

	bool updatePressureSensor();

};

#endif /* BME280HANDLING_H_ */
