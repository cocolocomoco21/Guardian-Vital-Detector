#include <avr/interrupt.h>

// Volatile variables (interrupt changes these) from pulse sensor
volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P =512;                      // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 525;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM

//Volatile variables for button
//volatile int b = 0;                     // used for state of button (currently unused, checkButton() call doesn't work)
const int buttonDebounce = 150;            // amount of time used to prevent debounce on the button
// was 20 for old code

void interruptSetup() {
	//FOR UNO
	// Initializes Timer1 to throw an interrupt every 2mS.
	TCCR1A = 0x00; // DISABLE OUTPUTS AND PWM ON DIGITAL PINS 9 & 10
	TCCR1B = 0x11; // GO INTO 'PHASE AND FREQUENCY CORRECT' MODE, NO PRESCALER
	TCCR1C = 0x00; // DON'T FORCE COMPARE
	TIMSK1 = 0x01; // ENABLE OVERFLOW INTERRUPT (TOIE1)
	ICR1 = 16000;  // TRIGGER TIMER INTERRUPT EVERY 2mS  
	//sei();         // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED     

	// EIMSK |= (1 << INT0);
	// EICRA |= (1 << ISC01); 
	//TODO why no digitalPinToInterrupt() here, why 0?
	// I guess 0 = pin2 and 1 = pin3
	attachInterrupt(1, buttonInterrupt, RISING);
	sei();
}


//ISR(EXT_INT0_vect){
void buttonInterrupt() {
	if (lastEmergencyCall != 0) {
		digitalWrite(emergencySuccessfulLEDPin, LOW);
		lastEmergencyCall = 0;
	}

	//static unsigned long lastButtonTime = 0;
	unsigned long buttonTime = millis();
	if (buttonTime - lastButtonTime > buttonDebounce) {
		//Serial.println(count);
		// Want the below commented?
		//cli();


		//lastButtonTime = buttonTime;
		/*
		TODO 
		Move lastButtonTime to be global variable?
		In loop() have condition that checks (millis() - lastButtonTime > 100) -> if it is blink, if not 
			Could do:

			volatile unsigned int lastButtonTime = 0;
			const int buttonBlinkTime = 100;		// blink button for 100 ms

			ISR (here){
				// ... all other code
				lastButtonTime = millis();		
				//TODO will this work? Will this be accurate? Using millis() inside ISR will give what exactly?
				// I think it will return the millis() value from right before the interrupt handler was called.
				// Since interrupts are disabled, the timer will not increment (?) and this millis value will be correct (?)
				// ... all other code
			}

			loop(){
				// If outside of buttonBlinkTime and the callLEDPin is still HIGH, write it LOW
				if ((millis() - lastButtonTime > buttonBlinkTime)){
					if (bitRead(PORTB, 4)) {					// callLEDPin is pin digital12, so this means PORTB4 -> PB4
						digitalWrite(callLEDPin, LOW);
					}
				}
				// If inside of buttonBlinkTime and callLEDPin is LOW, write it HIGH
				else if (!bitRead(PORTB, 4)){					// callLEDPin is pin digital12, so this means PORTB4 -> PB4
					digitalWrite(callLEDPin, HIGH);
				} else {
					// this should never happen. Inside of buttonBlinkTime and callLEDPin is HIGH
				}	
			}


			Modified:
			if ((millis() - lastButtonTime > buttonBlinkTime) && bitRead(PORTB, 4)) {	// callLEDPin is pin digital12, so this means PORTB4 -> PB4
				digitalWrite(callLEDPin, LOW);
			}
			// This should work since we set callLEDPin HIGH in the interrupt below, so code in loop simply looks when to shut it off
		
		*/

//		digitalWrite(callLEDPin, HIGH);
		

		// delays for 100000 microseconds, or 100 miliseconds
/*	
		for (int i = 0; i < 10; i++) {
			delayMicroseconds(10000);
		}
		digitalWrite(callLEDPin, LOW);
*/

		count++;
		//Serial.println(count);
		lastButtonTime = buttonTime;
		// Want the below commented?
		//sei();
	}
	//sei();
}

// THIS IS THE TIMER 1 INTERRUPT SERVICE ROUTINE. 
// Timer 1 makes sure that we take a reading every 2 miliseconds
ISR(TIMER1_OVF_vect){
//ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  if (noIntr == false) {
	  Signal = analogRead(pulsePin);              // read the Pulse Sensor 
	  sampleCounter += 2;                         // keep track of the time in mS with this variable
	  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

		//  find the peak and trough of the pulse wave
	  if (Signal < thresh && N >(IBI / 5) * 3) {       // avoid dichrotic noise by waiting 3/5 of last IBI
		  if (Signal < T) {                        // T is the trough
			  T = Signal;                         // keep track of lowest point in pulse wave 
		  }
	  }

	  if (Signal > thresh && Signal > P) {          // thresh condition helps avoid noise
		  P = Signal;                             // P is the peak
	  }                                        // keep track of highest point in pulse wave

	  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
	  // signal surges up in value every time there is a pulse
	  if (N > 250) {                                   // avoid high frequency noise
		  if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3)) {
			  Pulse = true;                               // set the Pulse flag when we think there is a pulse
			  digitalWrite(blinkPin, HIGH);                // turn on pin 13 LED
			  IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
			  lastBeatTime = sampleCounter;               // keep track of time for next pulse

			  if (secondBeat) {                        // if this is the second beat, if secondBeat == TRUE
				  secondBeat = false;                  // clear secondBeat flag
				  for (int i = 0; i <= 9; i++) {             // seed the running total to get a realisitic BPM at startup
					  rate[i] = IBI;
				  }
			  }

			  if (firstBeat) {                         // if it's the first time we found a beat, if firstBeat == TRUE
				  firstBeat = false;                   // clear firstBeat flag
				  secondBeat = true;                   // set the second beat flag
				  sei();                               // enable interrupts again
				  return;                              // IBI value is unreliable so discard it
			  }


			  // keep a running total of the last 10 IBI values
			  word runningTotal = 0;                  // clear the runningTotal variable    

			  for (int i = 0; i <= 8; i++) {                // shift data in the rate array
				  rate[i] = rate[i + 1];                  // and drop the oldest IBI value 
				  runningTotal += rate[i];              // add up the 9 oldest IBI values
			  }

			  rate[9] = IBI;                          // add the latest IBI to the rate array
			  runningTotal += rate[9];                // add the latest IBI to runningTotal
			  runningTotal /= 10;                     // average the last 10 IBI values 
			  BPM = 60000 / runningTotal;               // how many beats can fit into a minute? that's BPM!
			  QS = true;                              // set Quantified Self flag 
			  // QS FLAG IS NOT CLEARED INSIDE THIS ISR
		  }
	  }

	  if (Signal < thresh && Pulse == true) {   // when the values are going down, the beat is over
		  digitalWrite(blinkPin, LOW);            // turn off pin 13 LED
		  Pulse = false;                         // reset the Pulse flag so we can do it again
		  amp = P - T;                           // get amplitude of the pulse wave
		  thresh = amp / 2 + T;                    // set thresh at 50% of the amplitude
		  P = thresh;                            // reset these for next time
		  T = thresh;
	  }

	  if (N > 2500) {                           // if 2.5 seconds go by without a beat
		  thresh = 512;                          // set thresh default
		  P = 512;                               // set P default
		  T = 512;                               // set T default
		  lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
		  firstBeat = true;                      // set these to avoid noise
		  secondBeat = false;                    // when we get the heartbeat back
	  }
	  // TODO want this? Comment out to fix bugs?
  }
  sei();                                   // enable interrupts when youre done!
}// end isr





