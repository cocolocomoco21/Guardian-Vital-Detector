void placeCall() {
	//cli();
	Serial.println(F("|-------------------------------------|\n|   EMERGENCY CALL IS BEING PLACED    |\n|-------------------------------------|\n"));
	int loopCount = 0;
	delay(10);        // this delay is used to accurately print the above string (interrupts cut it short)
	//noInterrupts();   // disable interrupts to prevent button or pulse from occuring within the place call
	cli();

	for (int i = 0; i < 10; i++) {
		// Write Call LED high for 100000 microseconds, or 100 milliseconds
		digitalWrite(callLEDPin, HIGH);
		//tone(buzzerPin, 1000);
		while (loopCount < 10) {
			delayMicroseconds(10000);
			//delay(200);
			loopCount++;
		}
		loopCount = 0;

		// Write Call LED low for 100000 microseconds, or 100 milliseconds
		digitalWrite(callLEDPin, LOW);
		//noTone(buzzerPin);
		while (loopCount < 10) {
			delayMicroseconds(10000);
			//delay(200);
			loopCount++;
		}
		loopCount = 0;
		//delay(200);
		//loopCount++;
	}

	// Pass message by reference to get message, it is updated after this call
	// IF ISSUES: Uncomment below, comment out if statement
	//getMessageInfo(message);
	if (!getMessageInfo(message)) {
		count = 0;
		noIntr = false;
		sei();
		return;
	}

	//delay(10);
	//Serial.println(sizeof(message))

	sei();
	//interrupts();	//This must be before sendSMS call to successfully call

	// This is the actual sending, depending on offlineMode and restrictCall
	if (!offlineMode && !restrictCall) {
		noIntr = true;
		//TODO: Bug: Sometimes sendSMS fails however the text still goes through. This code is 
		// altered right now to let all calls be 'successes', however this should be looked at
		// in the future
		if (!fona.sendSMS(sendto, message)) {
			Serial.println(F("Failed to send message"));			
			count = 0;
			noIntr = false;
			sei();
			return;
			//TODO have this? Untested
		}
		else {
			Serial.print(F("Sent to: Guardian Vital Detector at "));
			Serial.println(sendto);
		}

//		fona.sendSMS(sendto, message);
//		Serial.print(F("Sent to: Guardian Vital Detector at "));
//		Serial.println(sendto);
		noIntr = false;
	}

	//Successful call beep
	if (!quietMode) {
		noIntr = true;
		fona.setPWM(1500);
		delay(25);
		fona.setPWM(1000);
		delay(25);
		fona.setPWM(1500);
		delay(25);
		fona.setPWM(1000);
		delay(25);
		fona.setPWM(0);
		noIntr = false;
	}

	digitalWrite(emergencySuccessfulLEDPin, HIGH);
	lastEmergencyCall = millis();
	//interrupts();     // reenable interrupts

	count = 0;
}


boolean getMessageInfo(char* message) {	//this updates values at address of message (update message w/o returning value)
	sei();

	// Get GPRS location  and time through fona.getGSMLoc()
	noIntr = true;
	char *parseArray[4];

	// check for GSMLOC (requires GPRS)
	if (!offlineMode) {
		unsigned long getGPRSBufferTime = millis();
		while (!fona.GPRSstate()) {
			if (millis() - getGPRSBufferTime >= 10000) {
				Serial.println(F("Location fetch timed out"));
				noIntr = false;
				return false;
			}
		}
		uint16_t returncode;
		if (!fona.getGSMLoc(&returncode, replybuffer, 250)){
			Serial.println(F("Failed to get location"));
		}
		if (returncode == 0) {
			Serial.println(replybuffer);
		}
		else {
			Serial.print(F("Fail code #")); Serial.println(returncode);
			// If needed, omit this return for debugging
			noIntr = false;
			return false;
		}

		// Parse the returned coordinates and time
		// (Ideally) This parsing should work regardless of if we use GSMLoc or GPS
		// Right now it is designed for only GSMLoc
		int parseCount = 0;

		char *token;
		token = strtok(replybuffer, ",");
		while (token != NULL) {
			parseArray[parseCount] = token;
			token = strtok(NULL, ",");
			parseCount++;
		}

		// Hard coded the time change. Change this based on data we get
		char timeStr[2];
		timeStr[0] = parseArray[3][0];
		timeStr[1] = parseArray[3][1];
		int timeInt = atoi(timeStr);
		if (timeInt < 5)
			timeInt = 24 - timeInt;		// Prevents negative times
		else
			timeInt = timeInt - 5;		// This changes the time to be correct

		parseArray[3][0] = (timeInt / 10) + '0';
		parseArray[3][1] = (timeInt % 10) + '0';
	}
	else {
		// offlineMode is true, hard code the information (Shoutlet GPS coordinates)
		//strcpy(parseArray[0], "-89.412448");
		parseArray[0] = "-89.5293418";
		//strcpy(parseArray[1], "43.072768");
		parseArray[1] = "43.0859648";
		//strcpy(parseArray[2], "04/15/2016");
		parseArray[2] =  "08/11/2016";
		//strcpy(parseArray[3], "N/A");
		parseArray[3] = "N/A";
	}	
	
	delay(20);
	Serial.print(F("\nSend to: Guardian Vital Detector at "));
	Serial.println(sendto);

	// Set default message content
	memset(message, 0, 255);
	strcpy(message, "Emergency exists at GPRS location:\n");

	strcat(message, "Longitude: ");
	strcat(message, parseArray[0]);
	
	strcat(message, "\nLatitude: ");
	strcat(message, parseArray[1]);

	strcat(message, "\nDate: ");
	strcat(message, parseArray[2]); 
	
	strcat(message, "\nTime: ");
	strcat(message, parseArray[3]);
	
	Serial.println(F("\n\t\tData loaded...\n"));
	Serial.println(message);
	Serial.println();

	noIntr = false;
	return true;
}

/*
String command(const char *toSend, unsigned long milliseconds) {
	String result;
	Serial.print("Sending: ");
	Serial.println(toSend);
	fona.println(toSend);
	unsigned long startTime = millis();
	Serial.print("Received: ");
	while (millis() - startTime < milliseconds) {
		if (fona.available()) {
			char c = fona.read();
			Serial.write(c);
			result += c;  // append to the result string
		}
	}
	Serial.println();  // new line after timeout.
	return result;
}
*/