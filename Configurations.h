#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/*Macros */
#define HIGH_VAL(x) ((unsigned)(x) >> 8)
#define LOW_VAL(x)  ((x) & 0xFF)

#define SPEED_DIR_CMD   	B01000000  // 01XXXXXX   	(010=Reverse   011=Forward)
#define SPEED_DIR_CMD2  	B01100000  // 01XXXXXX   	(010=Reverse   011=Forward)
#define	CV_WRITE_CMD		B11101100	// 11101100  	(CV Write - WriteByte)
#define	CV_ACCESS_CMD		B11100000	// 111XXXXX		(CV Access - WriteByte)
#define FUNC_GRP1_CMD		B10000000	// 100XXXXX
#define	FUNC_GRP2_A_CMD		B10110000	// 1011XXXX	  	(F5-F8)
#define	FUNC_GRP2_B_CMD		B10100000	// 1010XXXX		(F9-F12)

//Motor Pins
#define MOTOR_FORWARD_PIN 5
#define MOTOR_REVERSE_PIN 6

//Light Pins
#define FRONT_LED_PIN 7
#define REAR_LED_PIN 8
#define LED_EXTRA_PIN 4


#define  DCC_1_DELAY         58
#define  DCC_0_DELAY         100
#define  DCC_PORT            9
#define  PREAMBLE_BIT_COUNT  12    //preamble

//DCC Bit States
#define PREAMBLE_STATE    0
#define WAIT_INIT_BIT    1  // Init packet bit
#define FIRST_BYTE_STATE  2
#define WAIT_START1_BIT   3
#define SECOND_BYTE_STATE 4
#define WAIT_START2_BIT   5
#define THIRD_BYTE_STATE  6
#define WAIT_START3_BIT   7
#define FORTH_BYTE_STATE  8
#define WAIT_START4_BIT   9
#define FIFTH_BYTE_STATE  10
#define WAIT_START5_BIT   11

//Method signatures
void _acknowledge_command();
void build_speed_direction_dcc_frame(byte dcc_address, boolean dcc_forward, byte dcc_speed, int speed_step);
byte generate_command_pattern( boolean dcc_forward, byte dcc_speed , int speed_step);
void dccTester();
void show_dcc_bytes( byte command_byte, byte dcc_address, byte addr_cmd_xor );
void preamble_pattern();
void build_dcc_packets(byte byte1, byte byte2, byte byte3);
void build_dcc_packets(unsigned char *buffer, char length);
void analyzeDCC();
int readEEPROMData(int eepromValue);
void initConfig();

typedef union TFuncGroup{
		unsigned char _byte;
		struct {
			unsigned F1		:1;	// bit 0
			unsigned F2		:1; // bit 1
			unsigned F3		:1; // bit 2
			unsigned F4		:1; // bit 3
			unsigned FL		:1; // Lights control F0
		};

		struct {
			unsigned F5		:1;  // bit 0
			unsigned F6		:1;  // bit 1
			unsigned F7		:1;
			unsigned F8		:1;
			unsigned F9		:1;
			unsigned F10	:1;
			unsigned F11	:1;
			unsigned F12	:1;
		};	
} ;

#endif
