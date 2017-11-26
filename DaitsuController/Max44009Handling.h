/*
 * Max44009Handling.h
 *
 *  Created on: 27. 6. 2017
 *      Author: jindra
 */

#ifndef MAX44009HANDLING_H_
#define MAX44009HANDLING_H_

#include <core/MyMessage.h>
#include <Task.h>

class Max44009Handling : public Task {
private:
	uint8_t sensorId;
	MyMessage msg;

public:
	Max44009Handling(uint8_t sensorId);
	virtual ~Max44009Handling();

	void setup();

	void presentation();

	virtual void run();
};

#endif /* MAX44009HANDLING_H_ */
