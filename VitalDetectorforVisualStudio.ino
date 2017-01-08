/*
TODO for code:
1) Remove unused code to optimize RAM/SRAM usage (want below 70%)
	FONA (see (7) below), pulse sensor, button, variables, print statements
2) Make it look nice, remove unused code and comments
3) Look at heatbeat inconsistencies?
4) Comment code to understand what happens where
5) Reorganize classes?
6) All Serial.print(string) should be Serial.print(F())
7) Sort through FONA library code to remove what isn't needed -> optimize/reduce memory
8) Conflict with buzzer (tone library using PWM output on pins 3 and 11) and button
	interrupt (external interrupt declared on pin 3)?
9) Use Serial Monitor or comment out all print statements?

*/



/* This code is from Pulse Sensor Amped 1.4 to initialize the pulse sensor */

/*  Pulse Sensor Amped 1.4    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ----------------------
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 ----------------------       ----------------------  ----------------------
*/

// Copied from FONAtest
//#include <Tone.h>
//#include <EEPROM.h>
//#include <avr/pgmspace.h>
#include "Adafruit_FONA.h"
	//TODO check that these pin assignments are correct
#define FONA_RX 5 //6 //10 //3
#define FONA_TX 6 //5 //7 //8 //7 //9 //6
#define FONA_RST 4

// this is a large buffer for replies
char replybuffer[255];

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// TODO we can possibly do hardware serial to optimize memory usage
// From Arduino Reference pages:
// "Serial. It communicates on digital pins 0 (RX) and 1 (TX) as well as 
// with the computer via USB. Thus, if you use these functions, you cannot
// also use pins 0 and 1 for digital input or output."
// Since we use the serial monitor, we cannot use hardware serial since
// there will be a conflict on pins 0 and 1.

// Hardware serial is also possible!
//HardwareSerial *fonaSerial = &Serial1;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
// Use this one for FONA 3G
//Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

uint8_t type;

//  Variables from pulse sensor code
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 8;                 // pin to blink led at each beat
//int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

// Variables from button code
const int buttonPin = 3;     // the number of the pushbutton pin
//int buttonState = 0;         // variable for reading the pushbutton status

// Volatile variables for button code interrupt
volatile int count = 0;       // volatile int for the current count of button presses
volatile bool noIntr = false;

// Other variables defined by us
int callLEDPin = 12;     // pin for when a call is being placed
//int buttonLEDPin = 12;  // pin for LED to show button state
int beatCount = 0;
int emergencySuccessfulLEDPin = 13;
// TODO want this to be volatile since it can be set by button ISR?
volatile unsigned long lastEmergencyCall = 0;
volatile unsigned long lastButtonTime = 0;
const int buttonBlinkTime = 100;		// blink button for 100 ms
const unsigned long CANCEL_TIME = 5000;     // This gives a "delay" of 5 seconds
char sendto[21] = "6086204045";
char *message = (char*)malloc(140);
const char *APN_name = "wholesale"; // "spark.telefonica.com"; //"wholesale";

// Different operating mode
const bool offlineMode = false;		// Still use despite not having service
const bool quietMode = false; 
const bool restrictCall = false;

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.


void setup() {
	/* Setup copied from the pulse sensor */

	pinMode(blinkPin, OUTPUT);        // pin that will blink to your heartbeat!
   // pinMode(fadePin, OUTPUT);         // pin that will fade to your heartbeat!
	Serial.begin(115200);             // we agree to talk fast!

	//Initialize FONA
	FONASetup();

	// Initialize the pulse sensor and the button interrupts
	interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
	// IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE,
	// UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
	//   analogReference(EXTERNAL);

	/* Setup used for the button*/

	// initialize the pushbutton pin as an input:
	pinMode(buttonPin, INPUT);
	digitalWrite(buttonPin, HIGH);
	
	/* Setup used for the LED output pins*/

	// Initialize the buttonLED and callLED pins to output
	pinMode(emergencySuccessfulLEDPin, OUTPUT);
	pinMode(callLEDPin, OUTPUT);
	
	//POST beep
	if (!quietMode) {
		noIntr = true;
		fona.setPWM(500);
		delay(25);
		fona.setPWM(1000);
		delay(25);
		fona.setPWM(1500);
		delay(25);
		fona.setPWM(2000);
		delay(25);
		fona.setPWM(0);
		noIntr = false;
	}
	
	// count = -1?
	count = 0;
}

//  Where the Magic Happens
void loop() {

	// Heartbeat stuff
	if (QS == true) {    // A Heartbeat Was Found
	// BPM and IBI have been Determined
	// Quantified Self "QS" true when arduino finds a heartbeat
	dataAnalysis(BPM);
	//digitalWrite(blinkPin, HIGH);       // Blink LED, we got a beat.
	// delay(150);
	//digitalWrite(blinkPin, LOW);
	fadeRate = 255;                     // Makes the LED Fade Effect Happen
	// Set 'fadeRate' Variable to 255 to fade LED with pulse
	//serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.
	QS = false;                      // reset the Quantified Self flag for next time
	}

	// Commented out because we do not fade the blink pin
	//    ledFadeToBeat();                      // Makes the LED Fade Effect Happen
	//    delay(20);                             //  take a break
  

	// Turns off emergencySuccessfulLEDPin after 20 seconds of it being on
	if ((millis() - lastEmergencyCall) > 20000) {
		digitalWrite(emergencySuccessfulLEDPin, LOW);
		lastEmergencyCall = 0;
	}

	//checkButtonLED();

	// set this to count >= 1 ? 
	if (count >= 1) {
            
		Serial.print(F("\nEmergency Button Call Requested - "));
		count = 0;
		boolean isEmergency = emergency();

		if (isEmergency) {
			placeCall();
			beatCount = 0;
			//count = 0;
		}
		else {
			//Serial.print("Emergency Canceled\n");
			// uncomment if below count = 0 does not work
			//count = 0;
			//TODO no emergency, resume normal operation. Determine what to do from here
		}
		count = 0;
	}
  
	if (count > 2){
	Serial.println(F("\nButton Error. Resetting."));
	count = 0;
	}
}



void ledFadeToBeat() {
	fadeRate -= 15;                         //  set LED fade value
	fadeRate = constrain(fadeRate, 0, 255); //  keep LED fade value from going into negative numbers!
   // analogWrite(fadePin, fadeRate);         //  fade LED
}


boolean FONASetup() {
	//pinMode(FONA_TX, OUTPUT);
	//pinMode(FONA_RX, INPUT);

	// This should not be necessary, since FONASetup() is called before the pulse sensor interruptSetup(),
	// however it is here for redundancy
	noIntr = true;

	// Initialize FONA
	fonaSerial->begin(4800);
	if (!fona.begin(*fonaSerial)) {
		Serial.println(F("Couldn't find FONA"));
		while (1);
		// able to reset by toggling reset switch for 100ms
	}
	type = fona.type();

	//pinMode(FONA_RX, INPUT);
	//pinMode(FONA_TX, OUTPUT);

	//This is just printing for Serial Monitor visualization
	Serial.println(F("FONA is OK"));
	Serial.print(F("Found "));
	switch (type) {
	case FONA800L:
		Serial.println(F("FONA 800L")); break;
	case FONA800H:
		Serial.println(F("FONA 800H")); break;
	case FONA808_V1:
		Serial.println(F("FONA 808 (v1)")); break;
	case FONA808_V2:
		Serial.println(F("FONA 808 (v2)")); break;
	case FONA3G_A:
		Serial.println(F("FONA 3G (American)")); break;
	case FONA3G_E:
		Serial.println(F("FONA 3G (European)")); break;
	default:
		Serial.println(F("???"));
		return false;
		//break;
	}


	// Print module IMEI number.
	char imei[15] = { 0 }; // MUST use a 16 character buffer for IMEI!
	uint8_t imeiLen = fona.getIMEI(imei);
	if (imeiLen > 0) {
		Serial.print(F("Module IMEI: ")); Serial.println(imei);
	}


	// Set up/verify SIM 
	// read the CCID
	fona.getSIMCCID(replybuffer);  // make sure replybuffer is at least 21 bytes!
	Serial.print(F("SIM CCID = ")); Serial.println(replybuffer);


	// Set up/verify cellular
	// read the network/cellular status
	// TODO if n != 1 or 5, while loop here? Until network is registered?
	uint8_t n = fona.getNetworkStatus();
	Serial.print(F("Network status "));
	Serial.print(n);
	Serial.print(F(": "));
	if (n == 0) Serial.println(F("Not registered"));
	if (n == 1) Serial.println(F("Registered (home)"));
	if (n == 2) Serial.println(F("Not registered (searching)"));
	if (n == 3) Serial.println(F("Denied"));
	if (n == 4) Serial.println(F("Unknown"));
	if (n == 5) Serial.println(F("Registered roaming"));

	//TODO have this?
	digitalWrite(blinkPin, HIGH);

	// TODO uncomment to have FONA loop until it finds a network
	while (n != 1 && !offlineMode) {
		n = fona.getNetworkStatus();
		Serial.print(F("n: "));
		Serial.print(n);
	}

	//TODO have this?
	digitalWrite(blinkPin, LOW);

	if (!offlineMode) {
		Serial.print(F("Disable GPRS\n"));
		fona.enableGPRS(false);
		Serial.print(F("Enable GPRS\n"));
		fona.enableGPRS(true);

		// If using the Particle SIM card
		//fona.setGPRSNetworkSettings(F("spark.telefonica.com")); //, F("your username"), F("your password"));

		//TODO initilaize GPRS for Ting. May need to do this for different network if using different network
		FONAFlashStringPtr APN; // = F("Ting");
		APN = (FONAFlashStringPtr)APN_name; // "wholesale" or "spark.telefonica.com"
		const __FlashStringHelper *APN_p = APN;
		Serial.print(F("Registering APN\n"));
		fona.setGPRSNetworkSettings(APN_p);

		while (!fona.GPRSstate());

	}

	// enable network time sync
	if (!fona.enableNetworkTimeSync(true)) {
		Serial.println(F("Failed to enable network time"));
	}

	Serial.print(F("FONA is initialized\n"));
	delay(10);
	//TODO required?
	flushSerial();
	while (fona.available()) {
		Serial.write(fona.read());
	}

	noIntr = false;
	return true;
}

//TODO required?
void flushSerial() {
	while (Serial.available())
		Serial.read();
}

// Used to measure amount of SRAM used
int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}