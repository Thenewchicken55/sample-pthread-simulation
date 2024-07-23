#include <iostream>
#include <pthread.h>
#include <unistd.h>  // for usleep
#include <cstdlib>   // for std::rand and std::srand
#include <ctime>     // for std::time
#include <queue>
#include <vector>
#include <utility>   // for std::pair
#include <iomanip>   // for std::setw

typedef struct {
    int rid;
    const char* state; // can be waiting, riding, or wandering
    pthread_t* riderThread;
} Rider;

typedef struct {
    int cid;
    const char* state; // can be free or occupied
} Car;

// pthread_mutex_t rideTimerMutex;
// pthread_mutex_t wanderTimerMutex;
pthread_mutex_t lineMutex;
pthread_mutex_t ridersMutex;
std::vector<Rider*> riders;      // holds all riders
std::queue<Rider*> waitingLine; // bounded buffer queue of riders waiting for a car

pthread_mutex_t carMutex;
std::vector<Car> cars;          // holds all cars

bool useDisplayThread = true;
bool errorDisplay = false;

int currentTime = 0;

void printRiders() {
    if(!errorDisplay)
        return;

    std::cout << "\033[31m";
    pthread_mutex_lock(&ridersMutex);
    for (int i = 0; i < riders.size(); ++i) {
        std::cout << "Rider " <<  riders[i]->rid << " is " << riders[i]->state << "    ";
    }
    pthread_mutex_unlock(&ridersMutex);
    std::cout << std::endl;
    std::cout << "\033[0m";
}

void printCars() {
    if(!errorDisplay)
        return;

    std::cout << "\033[31m";
    for (int i = 0; i < cars.size(); ++i) {
        std::cout << "Car " <<  cars[i].cid << " is " << cars[i].state << "         ";
    }
    std::cout << std::endl;
    std::cout << "\033[0m";
}

void ride() {
    int Tbump = 3;

    // pthread_mutex_lock(&rideTimerMutex);
    // Generate random number between 0 and Tbump
    int random_time = std::rand() % (Tbump) + 1;

    if(errorDisplay)
    std::cout << "Rider is in the state of 'riding' for " << random_time << " seconds" << std::endl;
    // Sleep for the random time
    usleep(random_time * 1000000); // convert seconds to microseconds
    // pthread_mutex_unlock(&rideTimerMutex);
}

void wander() {
    int Twander = 5;

    // pthread_mutex_lock(&wanderTimerMutex);
    // Generate random number between 0 and Tbump
    int random_time = std::rand() % (Twander) + 1;

    if(errorDisplay)
    std::cout << "Rider is in the state of 'idle' for " << random_time << " seconds" << std::endl;
    // Sleep for the random time
    usleep(random_time * 1000000); // convert seconds to microseconds
    // pthread_mutex_unlock(&wanderTimerMutex);
}

void Bump(void* arg) {
    const int timeOut = 10;
    while(currentTime < timeOut) {
        if(errorDisplay){
        pthread_mutex_lock(&lineMutex);
        printRiders();
        printCars();
        pthread_mutex_unlock(&lineMutex);
        }

        // wait until there is a rider in the waiting line
        while(waitingLine.empty()) {}

        // get the rider from the front of the line
        pthread_mutex_lock(&lineMutex);
        Rider* rider = waitingLine.front();
        waitingLine.pop();
        pthread_mutex_unlock(&lineMutex);

        // check for a free car
        bool foundCar = false;
        int indexOfCar = 0;
        while (!foundCar) {
            pthread_mutex_lock(&carMutex);
            for (indexOfCar = 0; indexOfCar < cars.size(); ++indexOfCar) {
                if (cars[indexOfCar].state == "free") {
                    foundCar = true;
                    cars[indexOfCar].state = "occupied";
                    rider->state = "riding";
                    if(errorDisplay)
                    std::cout << "setting rider " << rider->rid << " to riding" << std::endl;
                    break;
                }
            }
            pthread_mutex_unlock(&carMutex);
        }

        Car* car = &cars[indexOfCar];

        if(errorDisplay){
            std::cout << std::endl << "Rider " << rider->rid << " found car " << car->cid << " to take" << std::endl;
            std::cout << "Rider is " << rider->state << "; Car is " << car->state << std::endl << std::endl;
        }
        // Rider goes for a ride
        ride();

        // rider finishes ride
        car->state = "free";
        rider->state = "wandering";

        if(errorDisplay)
        std::cout << "Rider " << rider->rid << " finished ride and car " << car->cid << " is now free" << std::endl;

        // rider wanders and gets food
        wander();

        //finished wandering now put rider back in line
        pthread_mutex_lock(&lineMutex);
        waitingLine.push(rider);
        pthread_mutex_unlock(&lineMutex);

        rider->state = "waiting";
    }
}

void* display(void* arg) {
    while (useDisplayThread){
        std::cout << currentTime << ": ";

        int offset = 0;
        if (currentTime >= 10)
            offset = 1;
        // Print the elements of the riders array

        // Print header
        std::cout << std::setw(10 - offset) << "Rider ID" << std::setw(12) << "State" << std::setw(15) << "isRiding?" << std::endl;
        std::cout << std::setw(40) << std::setfill('-') << "-" << std::setfill(' ') << std::endl;

        pthread_mutex_lock(&ridersMutex);
        // Print data rows
        for (int i = 0; i < riders.size(); ++i) {
            std::string ridingState = "false";
            if(riders[i]->state == "riding")
                ridingState = "true";
            std::cout << std::setw(10) << riders[i]->rid << std::setw(15) << riders[i]->state << std::setw(12) << ridingState << std::endl;
            // std::cout << std::setw(10) << riders[i]->rid << std::setw(15) << riders[i]->state << std::endl;
        }

        pthread_mutex_unlock(&ridersMutex);

        // print each car
        for (int i = 0; i < cars.size(); ++i) {
            std::cout << "Car " <<  cars[i].cid << " is " << cars[i].state << "\t";
        }
        std::cout << std::endl << std::endl ;

        if (currentTime == 10){
        std::cout << std::endl;
        std::cout << "CLOSING NOW THE PARK NOW!!" << std::endl;
        std::cout << "PLEASE FINISH YOUR RIDE!!" << std::endl;
        std::cout << "AND GET OUT!!" << std::endl;
        std::cout << std::endl;
        }

        sleep(1);

        ++currentTime;
    }
    return nullptr;
}

int main() {
    // Initialize random seed
    std::srand(std::time(nullptr));

    // initialize empty cars
    int Ncars = 3;
    for (int cid = 0; cid < Ncars; cid++) {
        Car car = {cid, "free"};
        cars.push_back(car);
    }

    int Nriders = 5;
    // for each rider, create a thread and call Bump
    for (int rid = 0; rid < Nriders; rid++) {
        pthread_t riderThread;
        Rider* rider = new Rider{rid, "waiting", &riderThread};
        std::cout << "\033[32m";
        if(errorDisplay)
        std::cout << "Rider " << rider->rid << " is waiting" << std::endl;
        std::cout << "\033[0m";

        pthread_mutex_lock(&ridersMutex);
        riders.push_back(rider);
        pthread_mutex_unlock(&ridersMutex);

        pthread_mutex_lock(&lineMutex);
        waitingLine.push((riders[rid]));
        pthread_mutex_unlock(&lineMutex);

        // the thread is created and the function Bump is called, Bump is type casted to correct type
        int status = pthread_create(rider->riderThread, NULL, reinterpret_cast<void* (*)(void*)>(Bump), NULL);

        // pthread_create() returns zero when successful
        if (status != 0) {
            std::cerr << "Failed to create thread" << std::endl;
            return 1;
        }
    }

    pthread_t displayThread;
    pthread_create(&displayThread, NULL, display, NULL);

    // wait for all threads to finish
    for (int rid = 0; rid < Nriders; rid++) {
        pthread_join(*riders[rid]->riderThread, NULL);
    }

    std::cout<< "All riders have finished riding\n" << std::endl;

    // kill the display thread when the other threads are done
    useDisplayThread = false;
    pthread_cancel(displayThread);


    // N cars reffered to as ID: cid

    // car threads
    // displaying/printing thread

    // line for bumper cars, bounded buffer or maybe queue

    // free bumper cars

    // getFood() wander activity after riding in bumper car
    // getFood() takes random time from 0 - T seconds

    // when done wandering, put rid into waiting area buffer
}