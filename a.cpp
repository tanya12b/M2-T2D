#include <iostream>  // Standard input-output stream
#include <stdio.h>   // Standard input-output functions
#include <unistd.h>  // Provides access to the POSIX operating system API
#include <stdlib.h>  // General utilities library
#include <thread>    // For multi-threading support
#include <mutex>     // For mutual exclusion synchronization primitive
#include <condition_variable>  // For inter-thread communication
#include <string>  // String manipulation functions
#include <queue>     // Standard queue implementation
#include <fstream>   // Input/output stream class to operate on files
#include <sstream>   // String stream class to operate on strings
#include <iomanip>   // Input/output manipulators
#include <algorithm> // General algorithms
#include <chrono>    // Time utilities

using namespace std;

mutex mutex1;  // Mutex for synchronization

// Struct for traffic data row
struct TrafficSignal
{
    int index;            // Index
    std::string timestamp; // Timestamp
    int traffic_light_id;          // Traffic light ID
    int num_of_cars;       // Number of cars
};

// Global variables
vector<int> rowIndex;           // Index
vector<int> trafficLightID;     // Traffic light ID
vector<int> numOfCars;      // Number of cars
vector<string> timestamps;    // Timestamp
int rows = 0;                 // Number of rows
int minute_ind = 3;        // Number of rows representing 5 minutes (assuming 4 rows per 5 minutes)

// Function to sort traffic light data
bool sortMethod(struct TrafficSignal first, struct TrafficSignal second)
{
    if (first.num_of_cars > second.num_of_cars)
        return true;
    return false;
}

// Function to print sorted traffic light data
void printSortedTraffic(const TrafficSignal* sortedTraffic, const std::string& timestamp)
{
    cout << "Traffic lights Ids sorted according to more congested at  " << timestamp << endl;
    cout << "Traffic Light" << setw(20) << "No of cars passed" << endl;
    for (int i = 0; i < 3; ++i) // Adjusted for only 3 traffic light IDs
    {
        cout << setw(3) << sortedTraffic[i].traffic_light_id << setw(20) << sortedTraffic[i].num_of_cars << endl;
    }
}

// Function to process traffic data
void processTrafficData(int producerThreadsNum, int consumerThreadsNum)
{
    // Initialize counters
    int producerCount = 0; // Producer count
    int consumerCount = 0; // Consumer count
    string lastTimestamp = "";

    // Initialize queue to store traffic light data
    queue<TrafficSignal> trafficSignalQueue;

    condition_variable producerCV, consumerCV; // Initializing condition variables for producer and consumer

    // Array to hold the current status of each of the 3 traffic lights
    TrafficSignal trafficLightStatus[3] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}}; // Adjusted for only 3 traffic light IDs

    // Start time
    auto start = chrono::high_resolution_clock::now();

    // Producer function
    auto produceData = [&]() {
        while (producerCount < rows) {
            unique_lock<mutex> lk(mutex1); // Locking until producer finishes processing 

            if (producerCount < rows) { // If count is less than the number of rows in the dataset 
                trafficSignalQueue.push(TrafficSignal{rowIndex[producerCount], timestamps[producerCount], trafficLightID[producerCount], numOfCars[producerCount]}); // Push into queue
                consumerCV.notify_all(); // Notifying consumer threads
                producerCount++;
            } else {
                producerCV.wait(lk, [&]{ return producerCount < rows; }); // If count is greater than the number of rows in the data set wait
            }

            lk.unlock(); // Unlock after processing
            sleep(rand()%3); // Simulate processing time
        }
    };

    // Consumer function
    auto consumeData = [&]() {
        while (consumerCount < rows) {
            unique_lock<mutex> lk(mutex1); // Lock until processing

            if (!trafficSignalQueue.empty()) {
                TrafficSignal sig = trafficSignalQueue.front(); // Getting the front elements of the queue

                // Check if 5 minutes have elapsed or if it's the last row
                if ((consumerCount % minute_ind == 0 && consumerCount > 0) || consumerCount == rows - 1) {
                    // Sort the traffic lights according to the number of cars
                    sort(trafficLightStatus, trafficLightStatus + 3, sortMethod); // Adjusted for only 3 traffic light IDs
                    printSortedTraffic(trafficLightStatus, sig.timestamp);
                }

                // Update the current status of each traffic light
                for (int i = 0; i < 3; ++i) { // Adjusted for only 3 traffic light IDs
                    if (trafficLightStatus[i].traffic_light_id == sig.traffic_light_id) {
                        trafficLightStatus[i] = sig;
                        break;
                    }
                }

                trafficSignalQueue.pop(); // Pop the data
                producerCV.notify_all(); // Notify producer
                consumerCount++;
            } else {
                consumerCV.wait(lk, [&]{ return !trafficSignalQueue.empty(); }); // If queue empty, wait until producer produces
            }

            lk.unlock(); // Unlock after processing
            sleep(rand()%3); // Simulate processing time
        }
    };

    // Create producer threads
    vector<thread> producerThreads(producerThreadsNum);
    for (auto& t : producerThreads) {
        t = thread(produceData);
    }

    // Create consumer threads
    vector<thread> consumerThreads(consumerThreadsNum);
    for (auto& t : consumerThreads) {
        t = thread(consumeData);
    }

    // Join producer threads
    for (auto& t : producerThreads) {
        t.join();
    }

    // Join consumer threads
    for (auto& t : consumerThreads) {
        t.join();
    }

    // End time
    auto end = chrono::high_resolution_clock::now();

    // Calculate elapsed time
    auto elapsedSeconds = chrono::duration_cast<chrono::seconds>(end - start);

    cout << "Total execution time is " << elapsedSeconds.count() << " seconds" << endl;
}

// Function to get data from file
void getTrafficData()
{
    ifstream infile("log.txt");

    if (infile.is_open())
    {
        std::string line;
        getline(infile, line); // Skipping header line

        while (getline(infile, line))
        {
            istringstream iss(line);
            string ind, timestamp, traffic_light_id, num_of_cars;
            getline(iss, ind, ',');
            getline(iss, timestamp, ',');
            getline(iss, traffic_light_id, ',');
            getline(iss, num_of_cars, '\n');

            // Convert timestamp to time and check if it's less than or equal to 20:55:00
            if (timestamp <= "20:55:00") {
                rowIndex.push_back(stoi(ind));
                timestamps.push_back(timestamp);
                trafficLightID.push_back(stoi(traffic_light_id));
                numOfCars.push_back(stoi(num_of_cars));

                rows++; // Increment row count
            }
        }
        infile.close();
    }
    else
    {
        cerr << "Sorry, cant open the file." << endl;
        exit(1);
    }
}

int main()
{
    // Read traffic data from file
    getTrafficData();
    cout << "Total 8 Thread using 4 for each producer and consumer " << endl;
    processTrafficData(7, 7);

    return 0;
}
