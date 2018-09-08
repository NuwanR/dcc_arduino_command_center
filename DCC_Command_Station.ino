/**
 *  Both DCC Command Station & Decoder together
 *
 *  Functions
 *  =========================
 *  - Currently supports hard-coded address
 *  - Supports 14, 28, 128 Speed steps
 *  - Command Center sends both function group 1 & 2 signals
 *
 *
 *  TODO
 *  - Need to support for CV Read/Write
 *  - Need to support writing for Permanent memory (EEPROM)
 *  - Need to Supports function group 1 & 2
 *
 */

#include "Arduino.h"
#include "Configurations.h"
#include "EEPROM.h"
#include "QList.h"

String incomingString;
char delimiters[] = ":";
char* valPosition;
boolean commandReceived = false;
boolean serialCommandReceived = false;
String dcc_address;
String dcc_speed;
boolean dcc_forward;
int bitSection;
String instruction;
boolean packet_generated;
int dccState;
String dccPacket;
String instructionType;
boolean debug = false;
boolean debug_decoder = false;
boolean sendDccSignal = false;
boolean isProcessed = true;
static String commandString;

unsigned long start;
unsigned long end;
unsigned long diff;

//Testing
unsigned long pulseWidth;
unsigned char receivedBit;
unsigned char bitsReceived;
unsigned char buffer[5];
char bytesReceived;
unsigned char preambleCount;
unsigned char direction;
TFuncGroup functiongroup;
TFuncGroup decoderFunctiongroup;

int cvValue;
int cvId;

unsigned char decoder_address;
QList<String> qList;

void setup() {
	noInterrupts();
	// disable global interrupts
	TCCR1A = 0;     // set entire TCCR1A register to 0
	TCCR1B = 0;     // same for TCCR1B

	// set compare match register to desired timer count:
	OCR1A = 15624;

	// turn on CTC mode:
	TCCR1B |= (1 << WGM12);

	// Set CS10 and CS12 bits for 1024 prescaler:
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS12);

	// enable timer compare interrupt:
	TIMSK1 |= (1 << OCIE1A);
	interrupts();

	Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps

	dcc_forward = true;
	packet_generated = false;
	bitSection = 1;
	dccState = 0;

	pinMode(DCC_PORT, OUTPUT);
	pinMode(13, OUTPUT);
	incomingString = "";
	/*pinMode(REAR_LED_PIN, OUTPUT);
	pinMode(LED_EXTRA_PIN, OUTPUT);

	pinMode(MOTOR_FORWARD_PIN, OUTPUT);
	pinMode(MOTOR_REVERSE_PIN, OUTPUT);*/

	functiongroup._byte = 0;
	direction = 1;

	/*if(readEEPROMData(1) == 255){
		EEPROM.update(1,3);
	}*/
	//EEPROM.update(1,3);

	//dcc_address = readEEPROMData(1);
	//decoder_address = readEEPROMData(1);

	dcc_address = 3;
	decoder_address = 3;
	//initConfig();
}

ISR(TIMER1_COMPA_vect) {
	Serial.println("Overflow");
	sendDccSignal = true;
}

void initConfig(){
	DCCSendResetData();
	DCCSendIdleData();
}

void dccBit(int bit) {
	switch (bit) {
	case 0:
		//dccPacket += "0";
		//Serial.print("0");
		digitalWrite(DCC_PORT, HIGH);
		delayMicroseconds(DCC_0_DELAY);
		digitalWrite(DCC_PORT, LOW);
		delayMicroseconds(DCC_0_DELAY+1);
		break;

	case 1:
		//dccPacket += "1";
		//Serial.print("1");
		digitalWrite(DCC_PORT, HIGH);
		delayMicroseconds(DCC_1_DELAY);
		digitalWrite(DCC_PORT, LOW);
		delayMicroseconds(DCC_1_DELAY+1);
		break;
	}
}

void loop() {
  unsigned long startDCC = micros();
	packet_generated = false;
	//dccPacket = "";
	start = millis();
	char c;

	if(Serial.available() > 0){
		c=Serial.read();
		if(c=='<'){                    // start of new command
			Serial.println("Started");
			commandString = "";
		}else if(c=='>'){               // end of new command
			Serial.println("Got it");
	    	qList.push_front(commandString);
		}else{
			commandString += c;
		}
	}

	/*for(int i=0; i< qList.size(); i++){
		Serial.println(qList.get(i));
	}*/
	// send data only when you receive data:
	if(sendDccSignal){
		// read the incoming byte:
		/*incomingString = Serial.readString();
		Serial.println(incomingString);

		char inSrting[incomingString.length() + 1];
		incomingString.toCharArray(inSrting, incomingString.length() + 1);*/

		for(int i=0; i< qList.size(); i++){
			incomingString = qList.get(i);
			char inSrting[incomingString.length() + 1];
			incomingString.toCharArray(inSrting, incomingString.length() + 1);


		//This initializes strtok with our string to tokenize
		valPosition = strtok(inSrting, delimiters);
		while (valPosition != NULL) {
			if(String(valPosition).equalsIgnoreCase("IDLE")){
				instructionType = "idle";
				break;
			}else if(String(valPosition).equalsIgnoreCase("RESET")){
				instructionType = "reset";
				break;
			}
			switch (bitSection) {
			case 1:
				dcc_address = valPosition;
				//Always clear the instruction
				instructionType = "";

				bitSection++;
				break;

			case 2:
				instruction = valPosition;

				if (instruction.equalsIgnoreCase("F")) {
					instructionType = "direction";
					dcc_forward = true;
				} else if (instruction.equalsIgnoreCase("R")) {
					instructionType = "direction";
					dcc_forward = false;
				} else if (instruction.equalsIgnoreCase("FUNC")) {
					instructionType = "function";
				} else if (instruction.equalsIgnoreCase("FUNC-2A")) {
					instructionType = "function2a";
				} else if (instruction.equalsIgnoreCase("FUNC-2B")) {
					instructionType = "function2b";
				} else if (instruction.equalsIgnoreCase("CVW")){
					instructionType = "cvw";
				}

				bitSection++;
				break;

			case 3:
				/*
				 * Case Types
				 * s/S - Speed Direction
				 * f/F - Function
				 * c/C - CV
				 * */

				String str((char*) valPosition);
				char command = str.charAt(0);
				String functionString;
				char *function;

				switch (command) {
				case 's':
				case 'S':
					dcc_speed = (str.substring(1, str.length()));
					break;

				case 'F':
				case 'f':
					functionString = (str.substring(1, str.length()));
					function = const_cast<char*>(functionString.c_str());


					switch (function[0]) {
					case '1':functiongroup.F1 ^= 1;break;
					case '2':functiongroup.F2 ^= 1;break;
					case '3':functiongroup.F3 ^= 1;break;
					case '4':functiongroup.F4 ^= 1;break;
					case 'L':functiongroup.FL ^= 1;break;
					case '5':functiongroup.F5 ^= 1;break;
					case '6':functiongroup.F6 ^= 1;break;
					case '7':functiongroup.F7 ^= 1;break;
					case '8':functiongroup.F8 ^= 1;break;
					case '9':functiongroup.F9 ^= 1;break;
					case '10':functiongroup.F10 ^= 1;break;
					case '11':functiongroup.F11 ^= 1;break;
					case '12':functiongroup.F12 ^= 1;break;
					}
				break;

					case 'c':
					case 'C':
						if(instructionType.equalsIgnoreCase("CVW")){
							cvId = (str.substring(2, str.length())).toInt();
							valPosition = strtok(NULL, delimiters);

							cvValue = (atoi(valPosition));
						}
					break;
				}
				bitSection = 1;
				break;
			}

			//Here we pass in a NULL value, which tells strtok to continue working with the previous string
			valPosition = strtok(NULL, delimiters);

		}
		commandReceived = true;
		valPosition = "";

		if (commandReceived) {
			if (instructionType.equals("direction")) {
				build_speed_direction_dcc_frame(dcc_address.toInt(), dcc_forward,dcc_speed.toInt(), 14);
				//_acknowledge_command();
			} else if (instructionType.equals("function")) {
				DCCSendFuncGroup1Data(dcc_address.toInt(), &functiongroup);
			} else if (instructionType.equals("function2a")) {
				DCCSendFuncGroup2Data(dcc_address.toInt(), &functiongroup, 1);
			} else if (instructionType.equals("function2b")) {
				DCCSendFuncGroup2Data(dcc_address.toInt(), &functiongroup, 2);
			} else if (instructionType.equals("cvw")){
				if(debug){
					Serial.print("cvValue : ");
					Serial.println(cvValue, BIN);
				}
				DCCSendCV(dcc_address.toInt(), cvId, cvValue);
			}else if (instructionType.equals("reset")) {
				DCCSendResetData();
			}else if (instructionType.equals("idle")) {
				DCCSendIdleData();
			}

			commandReceived = false;
		}else{
			Serial.println("DCCSendIdleData");
			DCCSendIdleData();
		}
		sendDccSignal = false;
		 end = millis();
		 diff = end - start;

	if (packet_generated) {
		//Uncomment to test the DCC Decoding
		//dccTester();
	}
 
		 if(debug){
		 unsigned long endDCC = micros();
		 unsigned long delta = end - startDCC;
		 Serial.print("Strat Time : ");
		 Serial.println(start);

		 Serial.print("End Time : ");
		 Serial.println(end);

		 Serial.print("Spent Time : ");
		 Serial.println(delta);
		 }
	} //End of qList loop
 } //End of DCC timer functions
}// End of loop

/* Command Pattern */
byte generate_command_pattern(boolean dcc_forward, byte dcc_speed,
		int speed_step) {
	byte command_byte;
	switch (speed_step) {
	case 14:
		command_byte = B01000000 | (dcc_forward << 5) | (dcc_speed & B00001111); //14 step - 00001111 / 28 step - 00011111(31)
		break;

	case 28:
		command_byte = (((dcc_speed & 0x1F) + 2) >> 1)
				| ((dcc_speed & 0x01) << 4);
		command_byte |= (dcc_speed & 0x80) >> 2;
		command_byte |= B01000000;
		break;
	}

	return command_byte;
}
;

/*Byte generator*/
void dccByte(unsigned char byte) {
	int i;

	for (i = 0; i < 8; i++) {
		//Serial.print(i);
		//Serial.print("-");
		if ((byte & 0x80) == 0x80) {          // check the high bit
			dccBit(1);
		} else {
			dccBit(0);
		}
		byte =  byte << 1;                 // shift for the next bit

		delayMicroseconds(5);
	}
}

/* Build the DCC frame */
void build_speed_direction_dcc_frame(byte dcc_address, boolean dcc_forward,
		byte dcc_speed, int speed_step) {

	//valid_frame = false;
	byte dcc_command = generate_command_pattern(dcc_forward, dcc_speed,
			speed_step);

	// Build up the bit pattern for the DCC frame
	unsigned char buffer[2];
	buffer[0] = dcc_address;
	buffer[1] = dcc_command;
	build_dcc_packets(buffer, 2);
	//valid_frame = true;
	packet_generated = true;
}

void build_dcc_packets(unsigned char *buffer, int length) {

	int i;
	unsigned char checkSum = 0;

	preamble_pattern();
	dccBit(LOW);
	for (i = 0; i < length; i++) {
		dccByte(buffer[i]);
		checkSum ^= buffer[i];
		if(debug){
			Serial.print("buffer '"+ (String)i +"' build_dcc_packets: ");
			Serial.println(buffer[i], BIN);

			Serial.print("checkSum build_dcc_packets: ");
			Serial.println(checkSum, BIN);
		}
		delayMicroseconds(5);
		dccBit(LOW);
	}

	if(debug){
			Serial.print("i ");
			Serial.println(i, DEC);
	}
	dccByte(checkSum);
	dccBit(HIGH);
	//Testing purpose
	//show_dcc_bytes(buffer[1], buffer[0], checkSum);
}

//Preamble pattern, 14 '1's
void preamble_pattern() {
	for (byte i = 0; i < PREAMBLE_BIT_COUNT; i++) {
		dccBit(1);
	}
}

void DCCSendCV(byte locoAddress, int CVNumber, int CVValue) {
	unsigned char buffer[4];

	buffer[0] = locoAddress;
	buffer[1] = CV_WRITE_CMD;
	buffer[2] = CVNumber;
	buffer[3] = CVValue;

	build_dcc_packets(buffer, 4);
	packet_generated = true;
}

void DCCSendFuncGroup1Data(byte locoAddress, TFuncGroup *FunctionGroup) {
	unsigned char buffer[2];

	buffer[0] = locoAddress;
	buffer[1] = (FunctionGroup->_byte & B00011111) | FUNC_GRP1_CMD;

	if(debug){
		Serial.print("buffer[1] DCCSendFuncGroup1Data :");
		Serial.println(buffer[1], BIN);
	}

	build_dcc_packets(buffer, 2);
	packet_generated = true;
}

void DCCSendFuncGroup2Data(byte locoAddress, TFuncGroup *FunctionGroup, unsigned char Group){
	unsigned char buffer[2];

	buffer[0] = locoAddress;

	if (Group == 1) { // function group 2a (F5-F8)
		buffer[1] = ((FunctionGroup->_byte)& 0b00001111) | FUNC_GRP2_A_CMD;
	}else { // function group 2b (F9-F12)
		buffer[1] = ((FunctionGroup->_byte >> 4)	& 0b00001111) | FUNC_GRP2_B_CMD;
	}

	if(debug){
			Serial.print("buffer[1] DCCSendFuncGroup2Data :");
			Serial.println(buffer[1], BIN);
	}

	build_dcc_packets(buffer, 2);
	packet_generated = true;
}

void DCCSendResetData(void){
	if(debug){
		Serial.println("DCCSendResetData");
	}
	char i;
	unsigned char dcc_buff[2];

	//for(i=0; i<2; i++) {
		dcc_buff[0] = 0;
		dcc_buff[1] = 0;
		build_dcc_packets(dcc_buff,2);
	//}
	packet_generated = true;
}

void DCCSendIdleData(void){
	if(debug){
			Serial.println("DCCSendIdleData");
	}
	Serial.println("DCCSendIdleData");
  unsigned char dcc_buff[2];
	dcc_buff[0] = 0xFF;
	dcc_buff[1] = 0;
	build_dcc_packets(dcc_buff,2);
	packet_generated = true;
}


void show_dcc_bytes(byte command_byte, byte dcc_address, byte addr_cmd_xor) {
  //if(debug){
  	Serial.print("Command byte         :");
  	Serial.println(command_byte, BIN);
  	Serial.print("Address byte         :");
  	Serial.println(dcc_address, BIN);
  	Serial.print("Error Correction Byte:");
  	Serial.println(addr_cmd_xor, BIN);
  //}
}

void _acknowledge_command() {

if(true){
	Serial.println("Command Received:");
	Serial.print("  Address:");
	Serial.println(dcc_address);
	Serial.print("  Direction: ");
	if (dcc_forward) {
		Serial.println("forward");
	} else {
		Serial.println("reverse");
	}
	Serial.print("  Speed:");
	Serial.println(dcc_speed);
  }
}
