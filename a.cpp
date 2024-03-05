#include <iostream>           // Library for standard input-output operations
#include <fstream>            // Library for file input-output operations
#include <vector>             // Library for dynamic arrays (vectors)
#include <queue>              // Library for queue data structure
#include <thread>             // Library for multi-threading support
#include <mutex>              // Library for mutual exclusion synchronization primitives
#include <condition_variable> // Library for condition variable synchronization primitives
#include <chrono>             // Library for time-related functionality
#include <unordered_map>      // Library for unordered map data structure
#include <algorithm>          // Library for various algorithms

using namespace std;

// Struct to represent traffic signal data
struct TrafficData
{
    string timestamp;   // Timestamp of the traffic data
    int trafficLightId; // ID of the traffic light
    int numCarsPassed;  // Number of cars passed during the timestamp
};

// Bounded buffer class for producer-consumer pattern
class BoundedBuffer
{
private:
queue<TrafficData> buffer; // Queue to store traffic data
 mutex mtx;                 // Mutex for mutual exclusion
 condition_variable cv;     // Condition variable for synchronization
size_t capacity;           // Capacity of the buffer

public:
    BoundedBuffer(size_t cap) : capacity(cap) {}

    void produce(const TrafficData &data)
    { 
    // Method to produce traffic data
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]
         { return buffer.size() < capacity; });
        buffer.push(data);
        cout << "Produced: " << data.timestamp << " " << data.trafficLightId << " " << data.numCarsPassed << endl; // Debugging statement
        cv.notify_all();
    }

    TrafficData consume()
    {
     // Method to consume traffic data
    unique_lock<mutex> lock(mtx);
      cv.wait(lock, [this]
     { return !buffer.empty(); });
    TrafficData data = buffer.front();
    buffer.pop();
    cout << "Consumed: " << data.timestamp << " " << data.trafficLightId << " " << data.numCarsPassed << endl; // Debugging statement
    cv.notify_all();
     return data;
    }
};

// Function to process traffic data and report top N congested traffic lights
void consumeTrafficData(BoundedBuffer &buffer, int numLights)
{
    unordered_map<int, int> congestionMap;
     // Map to store traffic light id and total cars passed
    int currentHour = -1;
    while (true)
    {
        TrafficData data = buffer.consume();
        congestionMap[data.trafficLightId] += data.numCarsPassed;

        // Check if an hour boundary is crossed
    if (stoi(data.timestamp.substr(0, 2)) != currentHour)
        {
            currentHour = stoi(data.timestamp.substr(0, 2));

            // Report top N most congested traffic lights
    cout << "Hour " << currentHour << " - Top " << numLights << " congested traffic lights:" << endl;
     vector<pair<int, int>> sortedLights(congestionMap.begin(), congestionMap.end());
     partial_sort(sortedLights.begin(), sortedLights.begin() + numLights, sortedLights.end(),                [](const pair<int, int> &a, const pair<int, int> &b)
          {
           return a.second > b.second; // Sort in descending order of cars passed
          });
      for (int i = 0; i < numLights && i < sortedLights.size(); ++i)
          {
          cout << "Traffic Light ID: " << sortedLights[i].first << ", Total Cars Passed: " << sortedLights[i].second << endl;
          }
            congestionMap.clear(); // Clear the map for the next hour
        }
    }
}

int main()
{
    const string filename = "log.txt"; // Path to the text file containing traffic data
    const int numLights = 3;           // Number of top congested traffic lights to report

    // Read traffic data from file
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error: Unable to open file: " << filename << endl;
        return 1;
    }
    vector<TrafficData> trafficData;
    TrafficData temp;
    while (file >> temp.timestamp >> temp.trafficLightId >> temp.numCarsPassed)
    {
        trafficData.push_back(temp);
    }
    file.close();

    BoundedBuffer buffer(48); // Bounded buffer with capacity for 48 rows (1 hour)
    thread consumer(consumeTrafficData, ref(buffer), numLights);

    // Simulate traffic data generation
    for (const auto &data : trafficData)
    {
        buffer.produce(data);
        this_thread::sleep_for(chrono::milliseconds(100)); // Adjust sleep time if needed
    }

    consumer.join();

    return 0;
}
