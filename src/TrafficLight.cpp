#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform vector modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); });

    // remove last vector element from queue
    T message = std::move(_queue.front());
    _queue.pop_front();

    return message; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    _queue.emplace_back(std::move(msg));
    _cond.notify_one(); // Notify client
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight(int intersectionID)
{
    _intersectionID = intersectionID;
    _currentPhase = TrafficLightPhase::red;
    _trafficQueue = std::make_unique< MessageQueue<TrafficLightPhase> >();

    std::cout << "Traffic light on intersection # " << _intersectionID << " thread id = " << std::this_thread::get_id() << " set to red " << std::endl;
    
}



void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        // simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (_trafficQueue->receive() == TrafficLightPhase::green)
        {
            std::cout << "Traffic light # " << _intersectionID << " has turned green" << std::endl;
            break;
        }
    }
    return;
}


TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases,                     this)); 
}


// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    std::random_device rdCycleDur;
    std::mt19937 eng(rdCycleDur());
    std::uniform_int_distribution<> distr(4,6);
    auto cycle_duration = std::chrono::seconds(distr(eng)).count();
    std::cout << "  traffic light # " 
              << _intersectionID 
              << " has cycle of " 
              << cycle_duration 
              << " seconds" 
              << std::endl;

    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
 
    //start stop watch
    lastUpdate = std::chrono::system_clock::now();
    // run traffic light
    while (true)
    {
        // sleep briefly to let cpu rest
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count();

        if (timeSinceLastUpdate >= cycle_duration)
        {
            // toggle light state
            switch (_currentPhase)
            {
            case red:
                _currentPhase = green;
                break;

            case green:
                _currentPhase = red;
                break;

            default:
                _currentPhase = red;
                break;
            }

            // move update to message queue using send
            _trafficQueue->send(std::move(_currentPhase));

            // reset stop watch
            lastUpdate = std::chrono::system_clock::now();
        } 
    }
    
}