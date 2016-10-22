/*
 PROJECT: MySensors / Quality of radio transmission 
 PROGRAMMER: AWI (MySensors libraries)
 DATE: 20160529/ last update: 20160530
 FILE: AWI_Send.ino
 LICENSE: Public domain

 Hardware: ATMega328p board w/ NRF24l01
    and MySensors 2.0
    
Special:
    
    
Summary:
    Sends a radio message with counter each  x time to determine fault ratio with receiver
Remarks:
    Fixed node-id & communication channel to other fixed node
    
Change log:
20160530 - added moving average on fail/ miss count, update to 2.0
*/


//****  MySensors *****
// Enable debug prints to serial monitor
#define MY_DEBUG 
#define MY_RADIO_NRF24                                  // Enable and select radio type attached
//#define MY_RF24_CHANNEL 80                                // radio channel, default = 76
// MIN, LOW, HIGH, MAX
// #define MY_RF24_PA_LEVEL RF24_PA_LOW

#define MY_NODE_ID 250
#define NODE_TXT "Q 250"                                // Text to add to sensor name

#define MY_PARENT_NODE_ID 0                         // fixed parent to controller when 0 (else comment out = AUTO)
#define MY_PARENT_NODE_IS_STATIC
#define MY_BAUD_RATE 9600


// #define MY_RF24_CE_PIN 7                             // Ceech board, 3.3v (7,8)  (pin default 9,10)
// #define MY_RF24_CS_PIN 8
#define DESTINATION_NODE 0                              // receiving fixed node id (default 0 = gateway)

#include <MySensors.h>  

// helpers
#define LOCAL_DEBUG

#ifdef LOCAL_DEBUG
#define Sprint(a) (Serial.print(a))                     // macro as substitute for print, enable if no print wanted
#define Sprintln(a) (Serial.println(a))                 // macro as substitute for println
#else
#define Sprint(a)                                       // enable if no print wanted -or- 
#define Sprintln(a)                                     // enable if no print wanted
#endif


// MySensors sensor
#define counterChild 0

// send constants and variables
int messageCounter = 0 ; 
const int messageCounterMax = 100 ;                     // maximum message counter value 
const unsigned counterUpdateDelay = 500 ;               // send every x ms and sleep in between

// receive constants and variables
boolean failStore[messageCounterMax] ;                  // moving average stores & pointers
int failStorePointer = 0 ;
boolean missedStore[messageCounterMax] ;
int missedStorePointer = 0 ;
int newMessage = 0 ;
int lastMessage = -1 ;
int missedMessageCounter = 0 ;                          // total number of messages in range (messageCounterMax)
int failMessageCounter = 0 ;                            // total number of messages in range (messageCounterMax)
uint8_t parent = 0 ;                                    // parent node-id 


// standard messages
MyMessage counterMsg(counterChild, V_PERCENTAGE);       // Send value


void setup() {

    for(int i= 0 ; i <  messageCounterMax ; i++){       // init stores for moving averages
        failStore[i] = true ;
        missedStore[i] = true ;
    }
    missedStorePointer = failStorePointer = 0 ;
    delay(1000);
}

void presentation(){
// MySensors
    present(counterChild, S_DIMMER, "Quality counter " NODE_TXT) ;  // counter uses percentage from dimmer value
}


void loop() {
    // Sprint("count:") ; Sprintln(messageCounter) ;
    Sprint("Parent: "); Sprint(parent); Sprint("       Fail "); Sprint(failMessageCounter); Sprint("   "); Sprint(getCount(failStore, messageCounterMax)); Sprintln("%");
    Sprint("Destination: "); Sprint(DESTINATION_NODE); Sprint("  Miss "); Sprint(missedMessageCounter); Sprint("   "); Sprint(getCount(missedStore, messageCounterMax)); Sprintln("%");


    missedStore[failStorePointer] = false  ;            // set slot to false (ack message needs to set) ; 
    boolean succes = failStore[failStorePointer] = send(counterMsg.setDestination(DESTINATION_NODE).set(failStorePointer), true);  // send to destination with ack
    if (!succes){
        failMessageCounter++ ; 
        Sprint("Fail on message: ") ; Sprint(failStorePointer) ;
        Sprint(" # ") ; Sprintln(failMessageCounter);
    }
    failStorePointer++ ;
    if(failStorePointer >= messageCounterMax){
        failStorePointer =  0   ;                       // wrap counter
    }
    parent = getParentNodeId();                         // get the parent node (0 = gateway)
    wait(counterUpdateDelay) ;                          // wait for things to settle and ack's to arrive
}

void receive(const MyMessage &message) {                // Expect few types of messages from controller
    newMessage = message.getInt();                      // get received value
    switch (message.type){
        case V_PERCENTAGE:
            missedStore[newMessage] = true ;            // set corresponding flag to received.
            if (newMessage > lastMessage){              // number of messages missed from lastMessage (kind of, faulty at wrap)
                Sprint("Missed messages: ") ; Sprintln( newMessage - lastMessage - 1) ;
                missedMessageCounter += newMessage - lastMessage - 1 ;
            }
            lastMessage = newMessage ;
            break ;
        default: break ;
    }
}


// calculate number of false values in array 
// takes a lot of time, but who cares...
int getCount(boolean countArray[], int size){
    int falseCount = 0 ;
    for (int i = 0 ; i < size ; i++){
        falseCount += countArray[i]?0:1 ;
    }
    return falseCount ;
}
