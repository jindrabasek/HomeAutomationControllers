/*
 * Veml6070Handling.h
 *
 *  Created on: 27. 6. 2017
 *      Author: jindra
 */

#ifndef VEML6070HANDLING_H_
#define VEML6070HANDLING_H_

#include <core/MyMessage.h>
#include <Task.h>

class Veml6070Handling: public Task {
private:
	uint8_t uvSensorId;
	uint8_t uviSensorId;
	MyMessage msgUv;
	MyMessage msgUvi;

public:
	Veml6070Handling(uint8_t uvSensorId, uint8_t uviSensorId);
	virtual ~Veml6070Handling();

	void setup();

	void presentation();

	virtual void run();
};

#endif /* VEML6070HANDLING_H_ */
