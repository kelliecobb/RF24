/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Revised by: Kellie Cobb (kelliecobb)
 */

/**
 * Rev0 of battery updates: receive one float battery value from Arduino
 * transceiver to Pi transceiver. Write to CSV file
 * and check for upload to application.
 */
#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()
#include <fstream>     // for file manipulation

using namespace std;

/****************** Linux ***********************/
// Radio CE Pin, CSN Pin, SPI Speed
// CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use their own pin numbering
// CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
// ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

// Generic:
RF24 radio(22, 0);
/****************** Linux (BBB,x86,etc) ***********************/
// See http://nRF24.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

float payload = 0.0;
bool loop = 1; // exit condition for infinite receiving loop

void slave();   // prototype of the RX node's behavior
void overwrite(float voltage); // prototype of writing to CSV file

// custom defined timer for evaluating transmission time in microseconds
struct timespec startTimer, endTimer;
uint32_t getMicros(); // prototype to get ellapsed time in microseconds

int main(int argc, char** argv) {

    // perform hardware check
    if (!radio.begin()) {
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }

    // to use different addresses on a pair of radios, we need a variable to
    // uniquely identify which address this radio will use to transmit
    bool radioNumber = 0; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // print example's name
    cout << argv[0] << endl;

    // Let these addresses be used for the pair
    uint8_t address[2][6] = {"1Node", "2Node"};
    // It is very helpful to think of an address as a path instead of as
    // an identifying device destination

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    int PAlevel = 0;
    cout << "Enter desired PA level:" << endl;
    cout << "0 = MIN" << endl << "1 = LOW" << endl;
    cout << "2 = HIGH" << endl << "3 = MAX" << endl;
    cin >> PAlevel;
    if (PAlevel == 0) {
      radio.setPALevel(RF24_PA_MIN);
    }
    else if (PAlevel == 1) {
      radio.setPALevel(RF24_PA_LOW);
    }
    else if (PAlevel == 2) {
      radio.setPALevel(RF24_PA_HIGH);
    }
    else if (PAlevel == 3) {
      radio.setPALevel(RF24_PA_MAX);
    }

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    // For debugging info
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // ready to execute program now
    while (loop) {
      slave();
    }
    return 0;
}

/**
 * make this node act as the receiver
 */
void slave() {

    radio.startListening();                                  // put radio in RX mode

    time_t startTimer = time(nullptr);                       // start a timer
    while (time(nullptr) - startTimer < 120) {               // use a 2 minute timeout
        uint8_t pipe;
        if (radio.available(&pipe)) {                        // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize();          // get the size of the payload
            radio.read(&payload, bytes);                     // fetch payload from FIFO
            overwrite(payload);                              // call function to write payload to text file
            
            startTimer = time(nullptr);                      // reset timer
        }
      }
    cout << "Nothing rececived after 2 minutes, still listening." << endl;
}

/**
 * write payload (lowest battery voltage value) to text file with current time and corresponding % battery left
 * */
void overwrite(float voltage) {
    time_t curr_time;
            curr_time = time(NULL);
            tm *tm_local = localtime(&curr_time);
    float percent = 0;
    if (voltage > 7.5) {
        percent = (voltage - 7.5) / 5.1 * 100;
    }
    cout << "Test before" << endl;
    
    std::ofstream ofs("BatteryUpdates.txt", std::ofstream::trunc);

    ofs << tm_local->tm_hour << ":" << tm_local->tm_min << ":" << tm_local->tm_sec << ", " << voltage << ", " << percent;

    ofs.close();
    
    cout << tm_local->tm_hour << ":" << tm_local->tm_min << ":" << tm_local->tm_sec << ", " << voltage << ", " << percent; // for debugging
    cout << "test after" << endl;
}

/**
 * Calculate the ellapsed time in microseconds
 */
uint32_t getMicros() {
    // this function assumes that the timer was started using
    // `clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);`

    clock_gettime(CLOCK_MONOTONIC_RAW, &endTimer);
    uint32_t seconds = endTimer.tv_sec - startTimer.tv_sec;
    uint32_t useconds = (endTimer.tv_nsec - startTimer.tv_nsec) / 1000;

    return ((seconds) * 1000 + useconds) + 0.5;
}

