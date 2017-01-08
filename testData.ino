// Declare constants
const int BEAT_THRESHOLD = 10;
int const LOW_HEARTBEAT_BOUND = 30;
int const HIGH_HEARTBEAT_BOUND = 200;
// High/low range is arbitrary for now. It depends on the activity (sick, exercise, etc.)

// Declare variable
boolean isOutOfBounds = false;
unsigned long timeOfLastBeat=0;



// Analyzes the data
void dataAnalysis(int input){
//  Serial.print(F("BPM: "));
//  Serial.println(input);
  // Tests if the heartbeat is in the valid range specified by the constants
  if (input < LOW_HEARTBEAT_BOUND || input > HIGH_HEARTBEAT_BOUND){
    
    // If the beat count is less than the threshold, increment
    if (beatCount < BEAT_THRESHOLD){
      //digitalWrite(callLEDPin, HIGH);
      //delay(50);
      //digitalWrite(callLEDPin, LOW);
      Serial.print(F("\tBPM: "));
      Serial.print(input);
      Serial.print(F(" - Consecutive Dangerous Reading: "));
      Serial.println(beatCount + 1);
      // why is this count = 0 here?
      count = 0;
      beatCount++;
      isOutOfBounds = true;
    }

    // If the count is equal to the threshold, there is an emergency. This handles the emergency
    else{
      //Serial.print("Emergency call exists in dataAnalysis\n");
      Serial.print(F("\nAutomatic Emergency Call - "));
      boolean isEmergency = emergency();
      if (isEmergency){
        //Serial.print("Emergency exists in dataAnalysis\n");
        placeCall();
        //TODO here is a placeCall() call, handle this correctly
      }
      beatCount = 0;
      // uncomment if placeCall's count = 0 doesnt work
      // count = 0;
    }    
  }

  // The heartbeat is valid, count is 0.
  else {
    beatCount = 0;
    // Simple for now, might want to account for bouncing data between bad and good
    isOutOfBounds = false;
    Serial.print(F("\tBPM: "));
    Serial.println(input);
  }
}

// Called in case of an emergency
boolean emergency() {
	Serial.println(F("Emergency Situation Exists\n"));
	
	//uint8_t SaveSREG = SREG;   // save interrupt flag
	if (!quietMode) {
		noIntr = true;
		fona.setPWM(500);
		noIntr = false;
	}
	//SREG = SaveSREG;

	digitalWrite(emergencySuccessfulLEDPin, HIGH); //callLEDPin?

	// Declare, initialize variables
	unsigned long lastLit = millis();
	unsigned long endTime = millis() + CANCEL_TIME;

	// Check if the button is double clicked (count = 2) within the given time using millis().
	// If so, cancel the emergency and return false.
	count = 0;
	//checkButtonLED();

	while (millis() < endTime) {
		// Have this be count == 2 ?
		//Serial.println(count);
		
		//checkButtonLED();
		
		digitalWrite(blinkPin, LOW);	//suppressed the blinking by the pulse LED
		if (count >= 2) {
			Serial.println(F("\nEmergency Canceled\n"));

			if (!quietMode) {
				noIntr = true;
				fona.setPWM(0);
				noIntr = false;
			}
//			delay(100);

			/*
			digitalWrite(callLEDPin, HIGH);
			delay(450);
			digitalWrite(callLEDPin, LOW);
			delay(250);
			digitalWrite(callLEDPin, HIGH);
			delay(450);
			digitalWrite(callLEDPin, LOW);
			//delay(100);
			*/

			count = 0;

			//flushSerial();
			return false;
		}

		if ((millis() - lastLit) >= 250){
			digitalWrite(emergencySuccessfulLEDPin, !digitalRead(emergencySuccessfulLEDPin)); //callLEDPin?
			lastLit = millis();
		}
		//checkButtonLED();
	}

	//checkButtonLED();

	digitalWrite(callLEDPin, LOW);
	digitalWrite(emergencySuccessfulLEDPin, LOW);
	count = 0;

	// The call was not canceled, so return true.
	//Serial.print("Emergency successful\n");
	//noTone(buzzerPin);

	flushSerial();
	
	if (!quietMode) {
		noIntr = true;
		fona.setPWM(0);
		noIntr = false;
	}
	return true;
}

void checkButtonLED() {
	if ((millis() - lastButtonTime > buttonBlinkTime) && bitRead(PORTB, 4)) {	// callLEDPin is pin digital12, so this means PORTB4 -> PB4
		digitalWrite(callLEDPin, LOW);
	}
}