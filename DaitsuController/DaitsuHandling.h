/*
 * DaitsuHandling.h
 *
 *  Created on: 22. 6. 2017
 *      Author: jindra
 */

#ifndef DAITSUHANDLING_H_
#define DAITSUHANDLING_H_

#include <Arduino.h>
#include <core/MyMessage.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <Task.h>
#include <cstdint>


class DaitsuHandling {
private:
	uint8_t daitsuControllSensId;
	uint8_t daitsuSendCmdSensId;
	uint8_t swingSensId;
	MyMessage msgDaitsuControll;
	MyMessage msgDaitsuSendCmd;
	MyMessage msgSwing;
	IRsend irsend;


public:
	DaitsuHandling(uint8_t daitsuControllSensId, uint8_t daitsuSendCmdSensId, uint8_t swingSensId, uint16_t IRsendPin);

	virtual ~DaitsuHandling();

	void setup();

	void presentation();

	void receive(const MyMessage &message);

};

#endif /* DAITSUHANDLING_H_ */
