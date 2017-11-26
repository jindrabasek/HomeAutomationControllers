/*
 * IrHandling.h
 *
 *  Created on: 22. 6. 2017
 *      Author: jindra
 */

#ifndef IRHANDLING_H_
#define IRHANDLING_H_

#include <Arduino.h>
#include <core/MyMessage.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <Task.h>
#include <cstdint>


class IrHandling  : public Task {
private:
	uint8_t sensorId;
	uint8_t storeSwitchId;
	uint8_t sendCodeId;
	MyMessage msgIrReceive;
	MyMessage msgStoreSwitch;
	MyMessage msgsendCode;
	MyMessage msgIrRecord;
	IRrecv irrecv;
	IRsend irsend;
	bool waitAfterReceive = false;


public:
	IrHandling(uint8_t sensorId, uint8_t storeSwitchId, uint8_t sendCodeId, uint16_t recvpin, uint16_t IRsendPin);

	virtual ~IrHandling();

	void setup();

	void presentation();

	void receive(const MyMessage &message);

	virtual void run();

private:

	byte lookUpPresetCode (decode_results *ircode);

#ifdef SUPPORT_RAW
	bool storeRawRCCode(byte index);
#endif

	bool storeKnownRCCode(byte index);

#ifdef SUPPORT_RAW
	void sendRawRCCode(byte index);
#endif

	void sendKnownRCCode(byte index);

	void dump(decode_results *results);

	void storeEeprom(uint16_t start, uint16_t len, byte *buf);

	void recallEeprom(uint16_t start, uint16_t len, byte *buf);

};

#endif /* IRHANDLING_H_ */
