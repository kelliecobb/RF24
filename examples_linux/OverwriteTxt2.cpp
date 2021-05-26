
#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()
#include <fstream>

using namespace std;
void overwrite(float voltage);

int main(int argc, char** argv) {
    
    float test = 2.233;
    overwrite(test);
    
    return 0;
}

void overwrite(float voltage) {
    time_t curr_time;
            curr_time = time(NULL);
            tm *tm_local = localtime(&curr_time);

    std::ofstream ofs("test.txt", std::ofstream::trunc);

    ofs << tm_local->tm_hour << ":" << tm_local->tm_min << ":" << tm_local->tm_sec << ", " << voltage;

    ofs.close();
}
