#include "C12832.h" //Display screen
#include "mbed.h" //Mbed
#include "http_request.h" //Network request
#include "network-helper.h" //Network helper
#include <json.hpp> //parsing json library
using json = nlohmann::json;

//Setting up LCD screen on the board
C12832 lcdscreen(D11, D13, D12, D7, D10);

//Setting up LED lights at bottom of the board
//con = red, ld2 = green, ld3 = blue
//used to signal network connection, red = network failed, green = network connected
DigitalOut con(LED1);
DigitalOut ld2(LED2);
DigitalOut ld3(LED3);

//Setting up  the joysticks on the board, making use of up down and fire
//Tying the joystick to interrupt functions
InterruptIn down(A3);
InterruptIn up(A2);
InterruptIn fire(D4);

//Setting up the speaker on the board
PwmOut spkr(D6);

//Setting up the threads to be used in the system
Thread thread;

//Setting up a timer
Timer t;

//Setting up a struct to share and modify data globaly across the application's functions
struct TrainData 
{ 
   json timetable; //stores the current data from the train network
   int pos; //stores the position of the joy stick
   NetworkInterface* net; //stores the network connection
   int count; //keeps track on when the display should be refreshed when the user moves the joystick
} td; 

//The function that is used to display data to the screen, it takes in a single int parameter which 
//determins which index in the array should be displayed on the screen
void displayData(int i) {
    
    //The screen makes use of 2 rows so I have 2 sets of variables to be displayed so that index i and i+1 can be displayed
    //simultaniously
    string destination_name1; //name of first train destination
    string aimed_departure_time1; //time the train plans to departed from specified platform
    int best_departure_estimate_mins1; //time the train plans to arrive at the specified platform
    string status1; //status of the train

    string destination_name2; //name of second train destination
    string aimed_departure_time2;  //time the train plans to departed from specified platform
    int best_departure_estimate_mins2; //time the train plans to arrive at the specified platform
    string status2;//status of the train
    
    string time_of_day; //time of day according the train network at the time of the API request
    
    int jsonListSize; //stores the size of the vector array list
    
    //check to make sure that there are values within the vector array list
    //if so then proceed - protects against index out of bounds errors
    if(!td.timetable["departures"]["all"].empty()) {
        
        //get the size of the vector array from the stored json file which contains the list of train journies
        jsonListSize = td.timetable["departures"]["all"].size();
        
        //wraps round the size of the vector array list when scrolling through the listed train times 
        //So with a list of size 5, it will go 1,2,3,4,5,1,2,3,4,5,1... etc...
        int first = i % jsonListSize; //Uses modulo on the i index to make sure that it stays within the bounds of the array size when scrolling with the joystick
        int second = (i+1) % jsonListSize; //gets second array entry using i+1 which will always get the next entry after i if there is one to be found
        
        //gets the time of day from the train time table
        time_of_day = td.timetable["time_of_day"];
        
        //gets the first destination name at the specified index
        destination_name1 = td.timetable["departures"]["all"].at(first)["destination_name"];
        //gets the first aimed departure time at the specified index
        aimed_departure_time1 = td.timetable["departures"]["all"].at(first)["aimed_departure_time"];
        //gets the first estimated departure time at the specified index
        best_departure_estimate_mins1 = td.timetable["departures"]["all"].at(first)["best_departure_estimate_mins"];
        //there is a 1min delay on the API call so we increment it by 1 to compensate for it
        best_departure_estimate_mins1 = best_departure_estimate_mins1 + 1;
        
        //gets the second destination name at the specified index
        destination_name2 = td.timetable["departures"]["all"].at(second)["destination_name"];
        //gets the second aimed departure time at the specified index
        aimed_departure_time2 = td.timetable["departures"]["all"].at(second)["aimed_departure_time"];
        //gets the second estimated departure time at the specified index
        best_departure_estimate_mins2 = td.timetable["departures"]["all"].at(second)["best_departure_estimate_mins"];
        //there is a 1min delay on the API call so we increment it by 1 to compensate for it
        best_departure_estimate_mins2 = best_departure_estimate_mins2 + 1;
        
        //checks the size of the vector array list, if it is greater than 1 then we want to use beoth lines of the display
        if(jsonListSize > 1) {
            lcdscreen.cls();
            
            lcdscreen.locate(0,2);
            //Displays the index position of the list with the destination, time of departure and extimated time until departure
            //Since index starts at 0 we add 1 to first/second so it displays starting from 1 for more human friendly read
            if(best_departure_estimate_mins1 < 1) { //check to make sure time doesn't go into negative value due to network delays
                lcdscreen.printf("%d %s %s 0m", first+1, destination_name1.c_str(), aimed_departure_time1.c_str()); 
            } else {
                lcdscreen.printf("%d %s %s %dm", first+1, destination_name1.c_str(), aimed_departure_time1.c_str(), best_departure_estimate_mins1); 
            }
            
            lcdscreen.locate(0,15);
            //Displays the index position of the list with the destination, time of departure and extimated time until departure
            //Since index starts at 0 we add 1 to first/second so it displays starting from 1 for more human friendly read
            if(best_departure_estimate_mins2 < 1) { //check to make sure time doesn't go into negative value due to network delays
                lcdscreen.printf("%d %s %s 0m", second+1, destination_name2.c_str(), aimed_departure_time2.c_str()); 
            } else {
                lcdscreen.printf("%d %s %s %dm", second+1, destination_name2.c_str(), aimed_departure_time2.c_str(), best_departure_estimate_mins2); 
            }
           //checks the size of the vector array list, if it is less than 1 but greater than 0 then we only want to use 1 line of the display screen
        } else if (jsonListSize > 0) {
            lcdscreen.cls();
            
            lcdscreen.locate(0,2);
            //Displays the index position of the list with the destination, time of departure and extimated time until departure
            //Since index starts at 0 we add 1 to first/second so it displays starting from 1 for more human friendly read
            if(best_departure_estimate_mins1 < 1) { //check to make sure time doesn't go into negative value due to network delays
                lcdscreen.printf("%d %s %s 0m", first+1, destination_name1.c_str(), aimed_departure_time1.c_str()); 
            } else {
                lcdscreen.printf("%d %s %s %dm", first+1, destination_name1.c_str(), aimed_departure_time1.c_str(), best_departure_estimate_mins1);  
            }
        } 
        //If the vector array list is empty then there were no trains running at this time
    } else {
        lcdscreen.cls();
        lcdscreen.locate(0,2);
        lcdscreen.printf("No tains running now"); //prints to screen
    }
    
}

//Turns the speaker sound on with a frequency of 1000Hz
void soundOn() {
    float frq = 1000;
    spkr.period(1.0/frq);
    spkr = 0.5;   
}

//Turns the speaker sound off
void soundOff() {
    spkr = 1;   
}

//Adjust the value of count and pos listed in the TrainData struct to scroll up
void scrollUp() {
    td.count = td.count - 2;
    td.pos = td.pos - 1;  //decrement to go up
}

//Adjust the value of pos listed in the TrainData struct to scroll down
void scrollDown() {
    td.pos = td.pos + 1; //increment to go down
}

//Gets the intial and starting response from the transport API
void getResponse(HttpResponse* res) {
    
    char * responseBody = new char [res->get_body_as_string().size()+1]; //screen a new char array to store the response with the size of the response
    strcpy (responseBody, res->get_body_as_string().c_str()); //copies response data to responseBody variable
    
    td.timetable = json::parse(responseBody); //parses the application/json file recieved from API called into C/C++ data structures such as strings, ints and vectors
    
    delete[] responseBody; //Delete objects to save on memory space as the json object can be quite large
    
    lcdscreen.cls();
    //Displays status of the connection and tells the user they can use the joystick to scroll
    lcdscreen.locate(0,2);
    lcdscreen.printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());
    lcdscreen.locate(0,15);
    lcdscreen.printf("USE: Joystick to scroll");
    
    ThisThread::sleep_for(2000); //wait for 2 seconds
    
    up.rise(&scrollUp); //Adds an interrupt onto the rise of the joystick up button
    down.rise(&scrollDown); //Adds an interrupt onto the rise of the joystick down button
    
    //Rise and fall applied to fire button which allows a sound beep for as long as it is pressed and then removed the sound when it is unclicked
    fire.rise(&soundOn); //Adds an interrupt onto the rise of the joystick center button
    fire.fall(&soundOff); //Adds an interrupt onto the fall of the joystick up button
    
    //Call to display data starting from the first index
    displayData(0); 
    
    //while loop which continues to listen in for user interrupt input and change the display screen accordingly
    while(1) {
        if(td.pos < 0) { //checks value to of pos to ensure it doesn't go into the negative values
           td.pos = 0;
           displayData(td.pos); //just refreshes the current screen with index 0 if it tries to go into a negative value
        }
        if(td.count < 0) { //checks value to of count to ensure it doesn't go into the negative values
           td.count = 0;
           displayData(td.pos);
        }
        if (td.count < td.pos) { //checks to see if the value of pos has been changed if so then we need to display a new screen with the new index
            td.count = td.count + 1; 
            displayData(td.pos); //just refreshes the current screen with index 0 if it tries to go into a negative value  
        }
        ThisThread::sleep_for(100); //100ms delay in the loop
    }
}

//Checks the intial network connection when the device is first powered on
//Displays a red light if the connection fails or green light if the connection succeeds
NetworkInterface* checkCon() {
    NetworkInterface* network = connect_to_default_network_interface();
    if (!network) {
        lcdscreen.printf("Cannot connect to the network");
        con = 0;
    } else {
        ld2 = 0;    
    }
    return network;
    
}

//Since I am dealing with real time data I also need to constantly listen to changes in data from the network API
//As such I need to make use of another while loop which required the use of another thread which will run in the background constly checking for information
//Every 20 seconds the thread will make a call to the transport api for new train data
void thread2() {
    while (1) {
        ThisThread::sleep_for(20000); //wait for 20 seconds
        
        {
            t.start(); //start the timer
            
            //makes a HTTP request to the transport api which shows live train data to and from the Moulsecoomb train station in Brighton
            HttpRequest* get_req = new HttpRequest(td.net, HTTP_GET, "http://transportapi.com/v3/uk/train/station/MCB/live.json?app_id=a300fcf3&app_key=7ab68047cea1eb9f2e38080373c14a8b&darwin=false&to_offset=PT01:00:00&train_status=passenger");
            HttpResponse* get_res = get_req->send(); //sends the HTTP request and raits for a response
            
            if (!get_res) { //check to see if the HTTP request failed
                lcdscreen.cls();
                lcdscreen.locate(0,2);
                lcdscreen.printf("Request failed - Trying Again");
                lcdscreen.locate(0,15);
                lcdscreen.printf("Thread2 - (EC - %d)", get_req->get_error());
                
                t.stop(); //stop timer
                
                ld2 = 1; //turns green light off
                con = 0; //displays red light on failed request
            } else { //if the HTTP request was sucessful then parse the body
                
                char * responseBody = new char [get_res->get_body_as_string().size()+1]; //screen a new char array to store the response with the size of the response
                strcpy (responseBody, get_res->get_body_as_string().c_str()); //copies response data to responseBody variable
                
                td.timetable = json::parse(responseBody); //parses the application/json file recieved from API called into C/C++ data structures such as strings, ints and vectors
                
                t.stop(); //stop the timer
                printf("The time taken was %f seconds | %d milliseconds\n", t.read(), t.read_ms()); //measure how quickly the request took
                con = 1;
                //flashes green light to indicate a successful refresh 
                ld2 = 1;
                ThisThread::sleep_for(200); //wait for 200ms
                ld2 = 0; 
                
                //Calls displayData to update the display screen with the new data obtained from API
                displayData(td.pos);
                
                //Delete objects to save on memory space as the json object can be quite large
                delete[] responseBody;
                delete get_req;
            }
        }
            
    }
}

//The main method
int main() {
    //Start by setting all LED lights to off
    con = 1; 
    ld2 = 1;
    ld3 = 1;
    
    //Intialise pos and count to 0
    td.pos = 0;
    td.count = 0;
    
    lcdscreen.cls();
    //Display informatio to use about what is happening while the board is setting up a network connection and making the HTTP request
    lcdscreen.locate(0,2);
    lcdscreen.printf("Loading Data For:");
    lcdscreen.locate(0,15);
    lcdscreen.printf("Moulsecoomb Train Station");
 
    NetworkInterface* network = checkCon(); //Getting a network connection
    td.net = network; //storing network connection in the struct which saves time as intialising a new connection takes a few seconds
    
    if(network) { //ensures there is a network connection before making the HTTP request, else error message from checkCon()
    
        thread.start(thread2); //starts the second thread
        
        {
            printf("\n----- HTTPS GET request -----\n");
            
            //makes a HTTP request to the transport api which shows live train data to and from the Moulsecoomb train station in Brighton
            HttpRequest* get_req = new HttpRequest(network, HTTP_GET, "http://transportapi.com/v3/uk/train/station/MCB/live.json?app_id=a300fcf3&app_key=7ab68047cea1eb9f2e38080373c14a8b&darwin=false&to_offset=PT01:00:00&train_status=passenger");
            HttpResponse* get_res = get_req->send(); //sends the HTTP request and raits for a response
            
            if (!get_res) { //check to see if the HTTP request failed
                lcdscreen.cls();
                lcdscreen.locate(0,2);
                lcdscreen.printf("HttpRequest failed");
                lcdscreen.locate(0,15);
                lcdscreen.printf("Main - (error code %d)", get_req->get_error());
            } else { //if the HTTP request was sucessful then parse the body
                printf("\n----- HTTPS GET response -----\n");
                getResponse(get_res);
                delete get_res;
                delete get_req;
            }
        }
    }
}
