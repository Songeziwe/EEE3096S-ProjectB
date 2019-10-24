/* Driver program to test the c++ mosquitto client */
//#include "TestMos.h"
#include <iostream>
#include <mosquitto.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <stdio.h>

// Macros
#define HOST "m14.cloudmqtt.com"
#define PORT 16797
#define KEEPALIVE 60
#define USERNAME "xkxabjtv"
#define PASSWORD "aY6G1txTZr-q"

using namespace std;

// Globals
struct mosquitto* mosq;
char topic[50] = "testTopic";

// Declare methods
int setup(void);
int subscribe(void);
int publish(void);
void on_message_callback(struct mosquitto *, void *, const struct mosquitto_message*);

// Entry point of the program
int main(){
    cout << "running..." << endl;
    if(setup() == -1){ return 0; }
    usleep(2000000); // wait 2 seconds for connection to finish
    if(subscribe() == -1){ return 0; }
    if(publish() == -1){ return 0; }

    // attach a message callback for handling messages from the broker
    mosquitto_message_callback_set(mosq, on_message_callback);

    // event loop for this client
    for(;;);

    switch(mosquitto_loop_stop(mosq, false)){
         case MOSQ_ERR_SUCCESS:
            cout << "Mosquitto loop stopped successfully." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Mosquitto loop stop: Input parameters were invalid." << endl;
            return -1;
        case MOSQ_ERR_NOT_SUPPORTED:
            cout << "Mosquitto loop stop: Thread support not available." << endl;
            return -1;
        default:
            cout << "Mosquitto loop stop: Some other error happened." << endl;
            return -1;
    }
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}

// Definitions

// This function must be the first one to be called inside main
int setup(void){
    mosquitto_lib_init();

    mosq = mosquitto_new(NULL, true, 0);
    if(mosq == NULL){
        cout << "Unable to create a mosquitto struct" << endl;
        return -1;
    }
    switch(mosquitto_username_pw_set(mosq, USERNAME, PASSWORD)){
        case MOSQ_ERR_SUCCESS:
            cout << "Username and password are valid." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Invalid username and/or passwaord." << endl;
            return -1;
        case MOSQ_ERR_NOMEM:
            cout << "Out of memory error during setup." << endl;
            return -1;
        default:
            cout << "Some other error happened during sign in" << endl;
            break;
    }
    // Connect to the online MQTT Broker
    switch(mosquitto_connect(mosq, HOST, PORT, KEEPALIVE)){
        case MOSQ_ERR_SUCCESS:
            cout << "Connected successfully." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Input parameters were invalid during connection." << endl;
            return -1;
        case MOSQ_ERR_ERRNO:
            cout << "Errno error occured during connection." << endl;
            return -1;
        default:
            cout << "Some other error happened during connection." << endl;
            return -1;
    }
     switch(mosquitto_loop_start(mosq)){
        case MOSQ_ERR_SUCCESS:
            cout << "Mosquitto loop started.." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Mosquitto loop: Input parameters were invalid." << endl;
            return -1;
        case MOSQ_ERR_NOT_SUPPORTED:
            cout << "Mosquitto loop: Thread support not available." << endl;
            return -1;
        default:
            cout << "Mosquitto loop: Some other error happened." << endl;
            return -1;
    }
    return 0;
}
// Subscription method to the online broker
int subscribe(void){
    int* mid        = NULL;
    const char* sub = "testTopic";
    int qos         = 0;
    int feedback = mosquitto_subscribe(mosq, mid, sub, qos);
    switch(feedback){
        case MOSQ_ERR_SUCCESS:
            cout << "Subscription is successfully." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Subscription parameters were invalid." << endl;
            return -1;
        case MOSQ_ERR_NOMEM:
            cout << "Out of memory condition occurred during subscription." << endl;
            return -1;
        case MOSQ_ERR_NO_CONN:
            cout << "Client is not connected to broker." << endl;
            return -1;
        case MOSQ_ERR_MALFORMED_UTF8:
            cout << "Subscriptin: Topic is not valid UTF-8" << endl;
            return -1;
        default:
            cout << "Subscription: Some other error occerred." << endl;
            return -1;
    }
    return 0;
}
// For message from the broker
void on_message_callback(struct mosquitto *, void *, const struct mosquitto_message* message){
    printf("Message from the broker with topic %s is %s\n", message->topic, (char*)message->payload);
}
// Publish method to the online broker
int publish(void){
    char payload[50] = "Zibondiwe";
    int payloadlen = strlen(payload);
    int qos = 0;
    bool retain = false;
    int* mid = NULL;
    int feedback = mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos, retain);
    switch(feedback){
        case MOSQ_ERR_SUCCESS:
            cout << "Publish is successfully." << endl;
            break;
        case MOSQ_ERR_INVAL:
            cout << "Publish parameters were invalid." << endl;
            return -1;
        case MOSQ_ERR_NOMEM:
            cout << "Out of memory condition occurred during publish." << endl;
            return -1;
        case MOSQ_ERR_NO_CONN:
            cout << "Publish: Client is not connected to broker." << endl;
            return -1;
        case MOSQ_ERR_MALFORMED_UTF8:
            cout << "Publish: Topic is not valid UTF-8" << endl;
            return -1;
        default:
            cout << "Publish: Some other error occerred." << endl;
            return -1;
    }
    return 0;
}
