/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 * Revised by: Kellie Cobb (kelliecobb)
 */

/**
 * Rev0 of battery updates: sample both battery voltages,
 * and send one float battery value from Arduino transceiver
 * to Pi transceiver
 */
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin

// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

float payload = 0.0; // payload stores battery voltage to be transmitted
float primBat = 0.0;
float secBat = 0.0;
float lowestBat = 0.0;

int count = 0; // counter for counting 1s interrupts up to 60s

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }

  // print example's introductory prompt
  Serial.println(F("RF24/examples/BatteryUpdatesR0"));

  // role variable is hardcoded to RX behavior, inform the user of this
  Serial.println(F("Ready to transmit!"));

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

  // additional setup specific to the node's role
  radio.stopListening();  // put radio in TX mode

  cli();//stop interrupts

  //set timer1 interrupt at 1Hz
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12 and CS10 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts

  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data

} // setup

// Timer1 interrupt 1Hz
// After counter reaches 58 (1 min), sample and transmit battery voltage
ISR(TIMER1_COMPA_vect){
  count++;
  if (count == 58) { // 1 minute has passed
    // This device is a TX node

    // Sample both batteries, determine lowest battery, and transmit that voltage
    primBat = SamplePrimaryBattery();
    secBat = SampleSecondaryBattery();

    if (primBat <= secBat) {
      payload = primBat;
    }
    else {
      payload = secBat;
    }

    // Transmit battery voltage
    unsigned long start_timer = micros();                    // start the timer
    bool report = radio.write(&payload, sizeof(float));      // transmit & save the report
    unsigned long end_timer = micros();                      // end the timer

    if (report) {
      Serial.print(F("Transmission successful! "));          // payload was delivered
      Serial.print(F("Time to transmit = "));
      Serial.print(end_timer - start_timer);                 // print the timer result
      Serial.print(F(" us. Sent: "));
      Serial.println(payload);                               // print payload sent

    } else {
      Serial.println(F("Transmission failed or timed out")); // payload was not delivered
      // FIXME: ERROR HANDLING?
    }
    count = 0;
}

void loop() {
  // Do nothing while waiting for 1min to pass
} // loop

/*******************************************************************************/
// Function: SAMPLE PRIMARY BATTERY
// Parameters: none
// Returns: float voltage1 (represents actual primary battery voltage)
// Error handling: nothing for now (FIXME: include range of appropriate values, error otherwise? keep sampling until in range)
float SamplePrimaryBattery() {
      // DEBUGGING: Serial.println("Sampling A1 for J2...");
   // read the input on analog pin for J2 (primary battery - BOARD_PWR):
   int sensorValue1 = analogRead(A1);
      // DEBUGGING: Serial.print("ADC value is: ");
      // DEBUGGING: Serial.println((int)sensorValue1);

   // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.15):
   float voltage1 = sensorValue1 * (5.0 / 1023.0);
      // DEBUGGING: Serial.print("J2 divided primary battery voltage is: ");
      // DEBUGGING: Serial.println((float)voltage1);

   //Convert to actual battery voltage
   voltage1 = voltage1 / (10/40.1);
      // DEBUGGING: Serial.print("J2 primary battery voltage is: ");
      // DEBUGGING: Serial.println((float)voltage1);
      // DEBUGGING: Serial.println();
   return voltage1;
}

// Function: SAMPLE SECONDARY BATTERY
// Parameters: none
// Returns: float voltage0 (represents actual secondary battery voltage)
// Error handling: nothing for now (FIXME: include range of appropriate values, error otherwise? keep sampling until in range)
float SampleSecondaryBattery() {
      // DEBUGGING: Serial.println("Sampling A0 for J1...");
   // read the input on analog pin for J1 (secondary battery):
   int sensorValue0 = analogRead(A0);
      // DEBUGGING: Serial.print("ADC value is: ");
      // DEBUGGING: Serial.println((int)sensorValue0);

   // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.15):
   float voltage0 = sensorValue0 * (5.0 / 1023.0);
      // DEBUGGING: Serial.print("J1 divided secondary battery voltage is: ");
      // DEBUGGING: Serial.println((float)voltage0);

   //Convert to actual battery voltage
   voltage0 = voltage0 / (10/40.1);
      // DEBUGGING: Serial.print("J1 secondary battery voltage is: ");
      // DEBUGGING: Serial.println((float)voltage0);
      // DEBUGGING: Serial.println();
   return voltage0;
}
