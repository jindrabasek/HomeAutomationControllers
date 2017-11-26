/*
 * IrHandling.cpp
 *
 *  Created on: 22. 6. 2017
 *      Author: jindra
 */

#include "IrHandling.h"

#include <core/MySensorsCore.h>

#include <hal/architecture/MyHwESP8266.h>

#include <IRremoteESP8266.h>  // https://github.com/z3t0/Arduino-IRremote/releases
// OR install IRRemote via "Sketch" -> "Include Library" -> "Manage Labraries..."
// Search for IRRemote b shirif and press the install button

#include <IRrecv.h>
#include <IRsend.h>
#include "core/MyMessage.h"

#include <Arduino.h>

#define min(a,b) ((a)<(b)?(a):(b))

#define IR_SUPPORT_UNKNOWN_CODES

#define MY_RAWBUF  100

const char * TYPE2STRING[] = { "UNKONWN", "RC5", "RC6", "NEC", "Sony",
		"Panasonic", "JVC", "SAMSUNG", "Whynter", "AIWA RC T501", "LG", "Sanyo",
		"Mitsubishi", "Dish", "Sharp", "Denon" };
#define Type2String(x)   TYPE2STRING[x < 0 ? 0 : x]
#define AddrTxt          F(" addres: 0x")
#define ValueTxt         F(" value: 0x")
#define NATxt            F(" - not implemented/found")

#define IR_PRESETS_OFFSET 20
#define IR_PRESET_NUMBER_KNOWN_ADDRESS (IR_PRESETS_OFFSET - 2)

#ifdef SUPPORT_RAW
// Raw or unknown codes requires an Arduino with a larger memory like a MEGA and some changes to store in EEPROM (now max 255 bytes)
// #define IR_SUPPORT_UNKNOWN_CODES
typedef struct {
		unsigned long value;           // The data bits if type is not raw
		int len;             // The length of the code in bits
		uint16_t codes[MY_RAWBUF];
		uint16_t count;
} IRCodeRaw;
#endif

typedef struct {
		decode_type_t type;            // The type of code
		unsigned long value;           // The data bits if type is not raw
		int len;             // The length of the code in bits
		unsigned int address;         // Used by Panasonic & Sharp [16-bits]
} IRCodeKnown;

#define           MAX_STORED_IR_CODES_KNOWN 10

#define           MAX_STORED_IR_CODES_LEN_ADJUST  10

#ifdef SUPPORT_RAW
#define           MAX_STORED_IR_CODES_RAW 4
#define IR_PRESET_NUMBER_RAW_ADDRESS (IR_PRESETS_OFFSET - 1)
IRCodeRaw StoredIRCodesRaw[MAX_STORED_IR_CODES_RAW];
#endif
IRCodeKnown StoredIRCodesKnown[MAX_STORED_IR_CODES_KNOWN];

decode_results ircode;

bool progModeEnabled = false;
//byte progModeId = NO_PROG_MODE;

// Manual Preset IR values -- these are working demo values
// VERA call: luup.call_action("urn:schemas-arduino-cc:serviceId:ArduinoIr1", "SendIrCode", {Index=15}, <device number>)
// One can add up to 240 preset codes (if your memory lasts) to see to correct data connect the Arduino with this plug in and
// look at the serial monitor while pressing the desired RC button
IRCodeKnown PresetIRCodesKnown[] = {
		{ SAMSUNG, 0xE0E040BF, 32, 0 },  // on/off        11
		{ SAMSUNG, 0xE0E0807F, 32, 0 },  // source
		{ SAMSUNG, 0xE0E020DF, 32, 0 },  // 1
		{ SAMSUNG, 0xE0E0A05F, 32, 0 },  // 2
		{ SAMSUNG, 0xE0E0609F, 32, 0 },  // 3
		{ SAMSUNG, 0xE0E010EF, 32, 0 },  // 4
		{ SAMSUNG, 0xE0E0906F, 32, 0 },  // 5
		{ SAMSUNG, 0xE0E050AF, 32, 0 },  // 6
		{ SAMSUNG, 0xE0E030CF, 32, 0 },  // 7
		{ SAMSUNG, 0xE0E0B04F, 32, 0 },  // 8
		{ SAMSUNG, 0xE0E0708F, 32, 0 },  // 9             21
		{ SAMSUNG, 0xE0E034CB, 32, 0 },  // TTX
		{ SAMSUNG, 0xE0E08877, 32, 0 },  // 0
		{ SAMSUNG, 0xE0E0C837, 32, 0 },  // previous
		{ SAMSUNG, 0xE0E0E01F, 32, 0 },  // vol +
		{ SAMSUNG, 0xE0E0D02F, 32, 0 },  // vol -
		{ SAMSUNG, 0xE0E0F00F, 32, 0 },  // mute
		{ SAMSUNG, 0xE0E0D629, 32, 0 },  // chanel list
		{ SAMSUNG, 0xE0E048B7, 32, 0 },  // ch +
		{ SAMSUNG, 0xE0E008F7, 32, 0 },  // ch -
		{ SAMSUNG, 0xE0E09E61, 32, 0 },  // home          31
		{ SAMSUNG, 0xE0E0F20D, 32, 0 },  // guide
		{ SAMSUNG, 0xE0E0F50A, 32, 0 },  // extra
		{ SAMSUNG, 0xE0E0F807, 32, 0 },  // info
		{ SAMSUNG, 0xE0E006F9, 32, 0 },  // up
		{ SAMSUNG, 0xE0E08679, 32, 0 },  // down
		{ SAMSUNG, 0xE0E0A659, 32, 0 },  // left
		{ SAMSUNG, 0xE0E046B9, 32, 0 },  // right
		{ SAMSUNG, 0xE0E016E9, 32, 0 },  // ok
		{ SAMSUNG, 0xE0E01AE5, 32, 0 },  // return
		{ SAMSUNG, 0xE0E0B44B, 32, 0 },  // exit          41
		{ SAMSUNG, 0xE0E036C9, 32, 0 },  // a
		{ SAMSUNG, 0xE0E028D7, 32, 0 },  // b
		{ SAMSUNG, 0xE0E0A857, 32, 0 },  // c
		{ SAMSUNG, 0xE0E06897, 32, 0 },  // d
		{ SAMSUNG, 0xE0E058A7, 32, 0 },  // settings
		{ SAMSUNG, 0xE0E0A45B, 32, 0 },  // subtitles
		{ SAMSUNG, 0xE0E0926D, 32, 0 },  // rec
		{ SAMSUNG, 0xE0E0629D, 32, 0 },  // stop
		{ SAMSUNG, 0xE0E0A25D, 32, 0 },  // prev
		{ SAMSUNG, 0xE0E0E21D, 32, 0 },  // play          51
		{ SAMSUNG, 0xE0E052AD, 32, 0 },  // pause
		{ SAMSUNG, 0xE0E012ED, 32, 0 }  // next           53
};
#define MAX_PRESET_IR_CODES_KNOWN  (sizeof(PresetIRCodesKnown)/sizeof(IRCodeKnown))
#ifdef SUPPORT_RAW
#define MAX_IR_CODES_COUNT (MAX_STORED_IR_CODES_RAW + MAX_STORED_IR_CODES_KNOWN + MAX_PRESET_IR_CODES_KNOWN)
#else
#define MAX_IR_CODES_COUNT (MAX_STORED_IR_CODES_KNOWN + MAX_PRESET_IR_CODES_KNOWN)
#endif
#define MAX_IR_CODES_KNWON_COUNT (MAX_STORED_IR_CODES_KNOWN + MAX_PRESET_IR_CODES_KNOWN)

IrHandling::IrHandling(uint8_t sensorId, uint8_t storeSwitchId, uint8_t sendCodeId, uint16_t recvpin, uint16_t IRsendPin) : Task(0),
		sensorId(sensorId), storeSwitchId(storeSwitchId), sendCodeId(sendCodeId),
		msgIrReceive(sensorId, V_IR_RECEIVE),
		msgStoreSwitch(storeSwitchId, V_STATUS),
		msgsendCode(sendCodeId, V_PERCENTAGE),
		msgIrRecord(sensorId, V_IR_RECORD),
		irrecv(recvpin), irsend(IRsendPin) {
	startAtEarliestOportunity();
}

IrHandling::~IrHandling() {
	// TODO Auto-generated destructor stub
}

void IrHandling::setup() {

	// Tell MYS Controller that we're NOT recording
	send(msgIrRecord.set(0));


	Serial.println(F("Recall EEPROM settings"));

#ifdef SUPPORT_RAW
	recallEeprom(IR_PRESETS_OFFSET, sizeof(StoredIRCodesRaw), (byte *) &StoredIRCodesRaw);

	for (byte index = 0; index < MAX_STORED_IR_CODES_RAW; index++) {
		int count = StoredIRCodesRaw[index].count;
		Serial.print(F("Stored raw: "));
		Serial.print(index);

		Serial.print(F(" "));
		Serial.print((unsigned long) StoredIRCodesRaw[index].value, HEX);
		Serial.print(F(" ("));
		Serial.print(StoredIRCodesRaw[index].len, DEC);
		Serial.println(F(" bits)"));

		Serial.print(F("Raw ("));
		Serial.print(count, DEC);
		Serial.print(F("): "));

		ESP.wdtFeed();

		for (int i = 0; i < count; i++) {
			if ((i % 2) == 1) {
				Serial.print(StoredIRCodesRaw[index].codes[i] * USECPERTICK, DEC);
			} else {
				Serial.print(-(int) StoredIRCodesRaw[index].codes[i] * USECPERTICK, DEC);
			}
			Serial.print(" ");
		}
		Serial.println("");

	}
#endif

	recallEeprom(IR_PRESETS_OFFSET
#ifdef SUPPORT_RAW
			+ sizeof(StoredIRCodesRaw) + 1
#endif
		, sizeof(StoredIRCodesKnown), (byte *) &StoredIRCodesKnown);

	for (byte index = 0; index < MAX_STORED_IR_CODES_KNOWN; index++) {
		Serial.print(F("Stored known: "));
		Serial.print(index);
		Serial.print(F(" "));
		Serial.print(StoredIRCodesKnown[index].type, DEC);
		Serial.print(F(" "));
		Serial.print(Type2String(StoredIRCodesKnown[index].type));

		Serial.print(F(" "));
		Serial.print((unsigned long) StoredIRCodesKnown[index].value, HEX);
		Serial.print(F(" ("));
		Serial.print(StoredIRCodesKnown[index].len, DEC);
		Serial.println(F(" bits)"));

		ESP.wdtFeed();
	}

	// Start the ir receiver
	irrecv.enableIRIn();
	irsend.begin();

	Serial.println(F("Init done..."));
}

void IrHandling::run() {
	// Send locally attech sensors data here
	if (!waitAfterReceive) {
		if (irrecv.decode(&ircode)) {
			dump(&ircode);
			if (progModeEnabled) {
				// If we are in PROG mode (Recording) store the new IR code and end PROG mode

				if (ircode.decode_type == UNKNOWN) {
#ifdef SUPPORT_RAW
					byte progModeId = loadState(IR_PRESET_NUMBER_RAW_ADDRESS);
					if (storeRawRCCode((progModeId + 1) % MAX_STORED_IR_CODES_RAW)) {
						// If sucessfull RC decode and storage --> also update the EEPROM
						storeEeprom(IR_PRESETS_OFFSET, sizeof(StoredIRCodesRaw), (byte *) &StoredIRCodesRaw);
						saveState(IR_PRESET_NUMBER_RAW_ADDRESS, (progModeId + 1) % MAX_STORED_IR_CODES_RAW);
						progModeEnabled = false;

						Serial.print(F("Stored raw "));
						Serial.print((progModeId) % MAX_STORED_IR_CODES_RAW);
						Serial.println();

						// Tell MYS Controller that we're done recording
						//send(msgIrRecord.set(0));
						send(msgStoreSwitch.set(0));
					}
#else
					Serial.print(F("Raw codes not supported"));
#endif

				} else {
					byte progModeId = loadState(IR_PRESET_NUMBER_KNOWN_ADDRESS);
					if (storeKnownRCCode((progModeId + 1) % MAX_STORED_IR_CODES_KNOWN)) {
						// If sucessfull RC decode and storage --> also update the EEPROM
						storeEeprom(IR_PRESETS_OFFSET
#ifdef SUPPORT_RAW
								+ sizeof(StoredIRCodesRaw) + 1
#endif
								, sizeof(StoredIRCodesKnown), (byte *) &StoredIRCodesKnown);
						saveState(IR_PRESET_NUMBER_KNOWN_ADDRESS, (progModeId + 1) % MAX_STORED_IR_CODES_KNOWN);
						progModeEnabled = false;

						Serial.print(F("Stored known "));
						Serial.print((progModeId) % MAX_STORED_IR_CODES_KNOWN);
						Serial.println();

						// Tell MYS Controller that we're done recording
						//send(msgIrRecord.set(0));
						send(msgStoreSwitch.set(0));
					}

				}

			} else {
				// If we are in Playback mode just tell the MYS Controller we did receive an IR code
				if (ircode.value != REPEAT) {
					// Look if we found a stored preset 0 => not found
					byte num = lookUpPresetCode(&ircode);
					if (num) {
						// Send IR decode result to the MYS Controller
						Serial.print(F("Found code for preset #"));
						Serial.println(num);
						send(msgIrReceive.set(num));
					}
				}
			}

			// Wait a while before receive next IR-code (also block MySensors receiver so it will not interfere with a new message)
			waitAfterReceive = true;
			setPeriodUs(500000);
		}
	} else {
		waitAfterReceive = false;
		setPeriodUs(0);
		startAtEarliestOportunity();
		// Start receiving again
		irrecv.resume();
	}

}

void IrHandling::presentation() {
	// Register a sensors to gw. Use binary light for test purposes.
	present(sensorId, S_IR);
	present(storeSwitchId, S_BINARY);
	present(sendCodeId, S_DIMMER);
}

void IrHandling::receive(const MyMessage &message) {
	//Serial.print(F("New message: "));
	//Serial.println(message.type);

	if (!waitAfterReceive) {
		if (message.type == V_STATUS && message.sensor == storeSwitchId) {

			if (message.getBool()) {
				send(msgStoreSwitch.set(1));
				progModeEnabled = true;
				Serial.println(F("Record new IR"));

			} else {
				send(msgStoreSwitch.set(0));
				progModeEnabled = false;
			}

			// Start receiving ir again...
			irrecv.enableIRIn();
		}


		/*if (message.type == V_IR_RECORD) { // IR_RECORD V_VAR1
			// Get IR record requets for index : paramvalue
			progModeId = message.getByte() % MAX_STORED_IR_CODES;

			// Tell MYS Controller that we're now in recording mode
			send(msgIrRecord.set(1));
			Serial.print(F("Record new IR for: "));
			Serial.println(progModeId);
		}*/

		else if (message.type == V_IR_SEND && message.sensor == sensorId) {
			// Send an IR code from offset: paramvalue - no check for legal value

			byte code = message.getByte();
			if (code > 0 && code < MAX_IR_CODES_COUNT) {
				Serial.print(F("Send IR preset: "));
				Serial.print(code);
#ifdef SUPPORT_RAW
				if (code < MAX_STORED_IR_CODES_RAW) {
					sendRawRCCode(code);
				} else {
#endif
					sendKnownRCCode(code);
#ifdef SUPPORT_RAW
				}
#endif

			}

			// Start receiving ir again...
			irrecv.enableIRIn();

		}

		else if (message.type == V_PERCENTAGE && message.sensor == sendCodeId) {
			// Send an IR code from offset: paramvalue - no check for legal value

			byte code = message.getByte();
			if (code > 0 && code <= MAX_IR_CODES_COUNT) {
				Serial.print(F("Send IR preset: "));
				Serial.print(code);
#ifdef SUPPORT_RAW
				if (code < MAX_STORED_IR_CODES_RAW) {
					sendRawRCCode(code);
				} else {
#endif
					sendKnownRCCode(code);
#ifdef SUPPORT_RAW
				}
#endif
			}

			// Start receiving ir again...
			irrecv.enableIRIn();
		}


	}
}

byte IrHandling::lookUpPresetCode(decode_results *ircode) {
	// Get rit of the RC5/6 toggle bit when looking up
	if (ircode->decode_type == RC5) {
		ircode->value = ircode->value & 0x7FF;
	}
	if (ircode->decode_type == RC6) {
		ircode->value = ircode->value & 0xFFFF7FFF;
	}

	if (ircode->decode_type == UNKNOWN) {
#ifdef SUPPORT_RAW
		for (byte index = 0; index < MAX_STORED_IR_CODES_RAW; index++) {
			if (StoredIRCodesRaw[index].value == ircode->value
					&& StoredIRCodesRaw[index].len == ircode->bits) {
				// The preset number starts with 1 so the last is stored as 0 -> fix this when looking up the correct index
				return index + 1;
			}
		}
#else
		return 0;
#endif

	}

	for (byte index = 0; index < MAX_STORED_IR_CODES_KNOWN; index++) {
		if ((StoredIRCodesKnown[index].type == ircode->decode_type)
				&& StoredIRCodesKnown[index].value == ircode->value
				&& StoredIRCodesKnown[index].len == ircode->bits) {
			// The preset number starts with 1 so the last is stored as 0 -> fix this when looking up the correct index
			return index + 1
#ifdef SUPPORT_RAW
					+ MAX_STORED_IR_CODES_RAW
#endif
					;
		}
	}

	for (byte index = 0; index < MAX_PRESET_IR_CODES_KNOWN; index++) {
		if (PresetIRCodesKnown[index].type == ircode->decode_type
				&& PresetIRCodesKnown[index].value == ircode->value
				&& PresetIRCodesKnown[index].len == ircode->bits) {
			// The preset number starts with 1 so the last is stored as 0 -> fix this when looking up the correct index
			return index + 1
#ifdef SUPPORT_RAW
					+ MAX_STORED_IR_CODES_RAW
#endif
					+ MAX_STORED_IR_CODES_KNOWN;
		}
	}
	// not found so return 0
	return 0;
}


#ifdef SUPPORT_RAW
// Stores the code for later playback
bool IrHandling::storeRawRCCode(byte index) {

	Serial.println(F("Received unknown code, saving as raw"));
	// To store raw codes:
	// Drop first value (gap)
	// As of v1.3 of IRLib global values are already in microseconds rather than ticks
	// They have also been adjusted for overreporting/underreporting of marks and spaces
	byte rawCount = min(ircode.rawlen, MY_RAWBUF);
	for (int i = 0; i < rawCount; i++) {
		StoredIRCodesRaw[index].codes[i] = ircode.rawbuf[i]; // Drop the first value
	};

	StoredIRCodesRaw[index].value = ircode.value;
	StoredIRCodesRaw[index].len = ircode.bits;
	StoredIRCodesRaw[index].count = rawCount;
	return true;
}
#endif

// Stores the code for later playback
bool IrHandling::storeKnownRCCode(byte index) {

	if (ircode.value == REPEAT) {
		// Don't record a NEC repeat value as that's useless.
		Serial.println(F("repeat; ignoring."));
		return false;
	}
	// Get rit of the toggle bit when storing RC5/6
	if (ircode.decode_type == RC5) {
		ircode.value = ircode.value & 0x07FF;
	}
	if (ircode.decode_type == RC6) {
		ircode.value = ircode.value & 0xFFFF7FFF;
	}

	StoredIRCodesKnown[index].type = ircode.decode_type;
	StoredIRCodesKnown[index].value = ircode.value;
	StoredIRCodesKnown[index].address = ircode.address; // Used by Panasonic & Sharp [16-bits]
	StoredIRCodesKnown[index].len = ircode.bits;
	Serial.print(F(" value: 0x"));
	Serial.println((unsigned long) ircode.value, HEX);
	return true;
}

#ifdef SUPPORT_RAW
void IrHandling::sendRawRCCode(byte index) {
	IRCodeRaw *pIr = &StoredIRCodesRaw[index % MAX_STORED_IR_CODES_RAW] ;

	// Assume 38 KHz
	irsend.sendRaw(pIr->codes, pIr->count, 38);
	Serial.println(F("Sent raw"));
}
#endif

void IrHandling::sendKnownRCCode(byte index) {
	IRCodeKnown *pIr = (
			(index <
#ifdef SUPPORT_RAW
					MAX_STORED_IR_CODES_RAW +
#endif
					MAX_STORED_IR_CODES_KNOWN) ?
					&StoredIRCodesKnown[index] :
					&PresetIRCodesKnown[index - (
#ifdef SUPPORT_RAW
							MAX_STORED_IR_CODES_RAW +
#endif
							MAX_STORED_IR_CODES_KNOWN)]);


	Serial.print(F(" - sent "));
	Serial.print(Type2String(pIr->type));
	if (pIr->type == RC5) {
		// For RC5 and RC6 there is a toggle bit for each succesor IR code sent alway toggle this bit, needs to repeat the command 3 times with 100 mS pause
		pIr->value ^= 0x0800;
		for (byte i = 0; i < 3; i++) {
			if (i > 0) {
				delay(100);
			}
			irsend.sendRC5(pIr->value, pIr->len);
		}
	} else if (pIr->type == RC6) {
		// For RC5 and RC6 there is a toggle bit for each succesor IR code sent alway toggle this bit, needs to repeat the command 3 times with 100 mS pause
		if (pIr->len == 20) {
			pIr->value ^= 0x10000;
		}
		for (byte i = 0; i < 3; i++) {
			if (i > 0) {
				delay(100);
			}
			irsend.sendRC6(pIr->value, pIr->len);
		}
	} else if (pIr->type == NEC) {
		irsend.sendNEC(pIr->value, pIr->len);
	} else if (pIr->type == SONY) {
		irsend.sendSony(pIr->value, pIr->len);
	} else if (pIr->type == PANASONIC) {
		irsend.sendPanasonic(pIr->address, pIr->value);
		Serial.print(AddrTxt);
		Serial.println(pIr->address, HEX);
	} else if (pIr->type == JVC) {
		irsend.sendJVC(pIr->value, pIr->len, false);
	} else if (pIr->type == SAMSUNG) {
		irsend.sendSAMSUNG(pIr->value, pIr->len);
	} else if (pIr->type == WHYNTER) {
		irsend.sendWhynter(pIr->value, pIr->len);
	} else if (pIr->type == AIWA_RC_T501) {
		irsend.sendAiwaRCT501(pIr->value);
	} else if (pIr->type == LG || pIr->type == SANYO
			|| pIr->type == MITSUBISHI) {
		Serial.println(NATxt);
		return;
	} else if (pIr->type == DISH) {
		// need to repeat the command 4 times with 100 mS pause
		for (byte i = 0; i < 4; i++) {
			if (i > 0) {
				delay(100);
			}
			irsend.sendDISH(pIr->value, pIr->len);
		}
	} else if (pIr->type == SHARP) {
		irsend.sendSharp(pIr->address, pIr->value);
		Serial.print(AddrTxt);
		Serial.println(pIr->address, HEX);
	} else if (pIr->type == DENON) {
		irsend.sendDenon(pIr->value, pIr->len);
	} else {
		// No valid IR type, found it does not make sense to broadcast
		Serial.println(NATxt);
		return;
	}
	Serial.print(" ");
	Serial.println(pIr->value, HEX);
}

// Dumps out the decode_results structure.
void IrHandling::dump(decode_results *results) {
	int count = results->rawlen;

	Serial.print(F("Received : "));
	Serial.print(results->decode_type, DEC);
	Serial.print(F(" "));
	Serial.print(Type2String(results->decode_type));

	if (results->decode_type == PANASONIC) {
		Serial.print(AddrTxt);
		Serial.print(results->address, HEX);
		Serial.print(ValueTxt);
	}
	Serial.print(F(" "));
	Serial.print((unsigned long) results->value, HEX);
	Serial.print(F(" ("));
	Serial.print(results->bits, DEC);
	Serial.println(F(" bits)"));

	if (results->decode_type == UNKNOWN) {
		Serial.print(F("Raw ("));
		Serial.print(count, DEC);
		Serial.print(F("): "));

		for (int i = 0; i < count; i++) {
			if ((i % 2) == 1) {
				Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
			} else {
				Serial.print(-(int) results->rawbuf[i] * USECPERTICK, DEC);
			}
			Serial.print(" ");
		}
		Serial.println("");
	}
}

void saveStateI(const uint16_t pos, const uint8_t value)
{
	hwWriteConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos, value);
}
uint8_t loadStateI(const uint16_t pos)
{
	return hwReadConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos);
}

// Store IR record struct in EEPROM
void IrHandling::storeEeprom(uint16_t start, uint16_t len, byte *buf) {
	Serial.print(F("To store : "));
	Serial.print(len);
	Serial.println(F(" "));

	saveStateI(start, len / MAX_STORED_IR_CODES_LEN_ADJUST);
	for (uint16_t i = start + 1; i < start + len; i++, buf++) {
		saveStateI(i, *buf);
	}
}

void IrHandling::recallEeprom(uint16_t start, uint16_t len, byte *buf) {
	if (loadStateI(start) != len / MAX_STORED_IR_CODES_LEN_ADJUST) {
		Serial.println(F("Corrupt EEPROM preset values and Clear EEPROM"));
		saveStateI(start, len / MAX_STORED_IR_CODES_LEN_ADJUST);
		for (uint16_t i = start + 1; i < start + len; i++, buf++) {
			*buf = 0;
			saveStateI(i, *buf);
		}
		return;
	}
	for (uint16_t i = start + 1; i < start + len; i++, buf++) {
		*buf = loadStateI(i);
	}
}
