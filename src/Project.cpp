/** Enviromental Logger
  *
  * @author SBSSON002	MBKNTS002
  */

#include "Project.h"

/*
#ifdef RASPBERRY
 #include <BlynkApiWiringPi.h>
#else
 #include <BlynkApiLinux.h>
#endif

#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);
#include <BlynkWidgets.h>
*/
using namespace std;

// Have to subscribe only to the dissmiss and monitor topics
// in order to dissmiss the alarm
// monitor topic is for start/stop monitoring
const char* dissmiss = "dissmiss";
const char* monitor  = "monitor";

// Globals
long lastInterruptTime = 0;
int RTC;
int SPI;

int hours = 0;
int mins  = 0;
int secs  = 0;

int temp        = 0;
float voltage   = 0;
float v_out     = 0;
int light       = 0;
int humidity  = 0;
int interval    = 1;
bool monitoring = false;
bool restarted = false;
bool dissmissed = false;
int minutes = 0;
int hour = 0;
int seconds = 0;

int main(){
    if(initGPIO() == -1){
        cout << "Setup failed: initGPIO()" << endl;
        return 0;
    }
    // Setup MQTT
    if(setup() == -1){ return 0; }
    usleep(2000000); // wait 2 seconds for connection to finish
    if(subscribe() == -1){ return 0; }
    if(publish() == -1){ return 0; }

    // attach a message callback for handling messages from the broker
    mosquitto_message_callback_set(mosq, on_message_callback);

    // Initialize thread with parameters
    // give the thread priority of 99
    pthread_attr_t tattr;
    pthread_t thread_id;
    int newprio = 99;
    sched_param param;

    pthread_attr_init(&tattr);
    pthread_attr_getschedparam(&tattr, &param);
    param.sched_priority = newprio;
    pthread_attr_setschedparam(&tattr, &param);
    pthread_create(&thread_id, &tattr, readFromADC, (void*)1);

    // event loop
   // printf(" RTC Time | Sys Timer | Humidity | Temperature | Light | DAC out | Alarm\n");
    //printf("_____________________________________________________________________________\n");

    for(;;){
        // Print data to screen 
        /*if((v_out < 0.65 || v_out > 2.65) && !dissmissed){
             printf(" | %d:%d:%d  | %.1f V    | %d C        | %d     | %.2f V     |  %s   \n", hour, minutes, seconds, humidity, temp, light, v_out, "*");
             printf("_____________________________________________________________________________\n");
             lightLED(secs);
        }else{
            printf(" | %d:%d:%d  | %.1f V    | %d C        | %d     | %.2f V     |  %s   \n", hour, minutes, seconds, humidity, temp, light, v_out, "");
            printf("_____________________________________________________________________________\n");
        }*/
        //wiringPiI2CRead(RTC);
        //int wiringPiI2CReadReg8 (int fd, int reg);
        systemTime();
        //if(!restarted){ 
            hours = getHours();
            mins = getMins();
            secs = getSecs();
        //}
        hours = hFormat(hours);
        hours = decCompensation(hours);
        mins = decCompensation(mins);
        //secs += interval;
        secs = decCompensation(secs);

        wiringPiI2CWriteReg8(RTC,HOUR,hours);
        wiringPiI2CWriteReg8(RTC,MIN,mins);
        wiringPiI2CWriteReg8(RTC,SEC,secs);

        //hours = hexCompensation(hours);
        //mins = hexCompensation(mins);
        //secs = hexCompensation(secs);

        //mins %=60;
        //secs %=60;
       // printf("%x:%x:%x",hours,mins,secs);
        hours = wiringPiI2CReadReg8 (RTC, HOUR);
        mins  = wiringPiI2CReadReg8 (RTC, MIN);
        secs  = wiringPiI2CReadReg8 (RTC, SEC);

        tick();

        delay(interval + 1000);
    } // END OF EVENT LOOP

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

    pthread_join(thread_id, NULL);
    pthread_exit(NULL);
    return 0;
}

//PWM signal
void lightLED(int units){
    int brightness = (units/60)*512;
    //secs = decCompensation(secs);

    if(units == 0 || units == 60)
        pwmWrite(secs, 0);
    else {
        pwmWrite(secs,(brightness+1));
    }
}
/////////// Interrupt handlers /////////////////
void start_stop_monitoring(void){
    long interruptTime = millis();
    if(interruptTime - lastInterruptTime > 200){
        monitoring = !monitoring;
        restarted = !restarted;
        cout << "start/stop" << endl;
    }
    lastInterruptTime = interruptTime;
}
void dissmiss_alarm(void){
    long interruptTime = millis();
    if(interruptTime - lastInterruptTime > 200){
        dissmissed = !dissmissed;
    }
    lastInterruptTime = interruptTime;
}
void reset_system_time(void){
    long interruptTime = millis();
    if(interruptTime - lastInterruptTime > 200){
        hour = 0;
        minutes = 0;
        seconds = 0;
    }
    lastInterruptTime = interruptTime;
}
void change_reading_interval(void){
    long interruptTime = millis();
    if(interruptTime - lastInterruptTime > 200){
        //cout << "change interval pressed" << endl;
        switch(interval){
            case 1:
                ++interval;
                break;
            case 2:
                interval = 5;
                break;
            default:
                interval = 1;
                break;
        }
    }
    lastInterruptTime = interruptTime;
}
/////////// Setup //////////////
int initGPIO(void){
    wiringPiSetup();
    // Setup buttons
    for(size_t i = 0; i < 4;++i){
        pinMode(BTNS[i], INPUT);
        pullUpDnControl(BTNS[i], PUD_DOWN);
    }

    // attach interrupts to buttons
    wiringPiISR(BTNS[0], INT_EDGE_FALLING, start_stop_monitoring);
    wiringPiISR(BTNS[1], INT_EDGE_FALLING, dissmiss_alarm);
    wiringPiISR(BTNS[2], INT_EDGE_FALLING, reset_system_time);
    wiringPiISR(BTNS[3], INT_EDGE_FALLING, change_reading_interval);

    // LED PWM
    pinMode(5, OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(1024);


    // Setup RTC
    RTC = wiringPiI2CSetup(RTCAddr);
    if(RTC == -1){
        cout << "Failed setup of RTC" << endl;
        return RTC;
    }

    // Setup SPI interface for ADC
    SPI = wiringPiSPISetup(SPI_CHAN, SPI_SPEED);
    if(SPI == -1){
        cout << "Failed setup of SPI interface" << endl;
        return SPI;
    }

    // Setup ADC
    mcp3004Setup(BASE, SPI_CHAN);
    cout << "Setup done." << endl;
    return 0;
}
///////// Thread to read from ADC //////////////
void *readFromADC(void *threadArgs){
    while(!monitoring)
        continue;
    // read from ADC iff monitoring is true
    while(monitoring){
        // Read temperature and convert to degrees celcius
        temp    = analogRead(BASE);
        voltage = ((float)temp / 1023) * 3.3;
        temp    = ((voltage * 1000) - 500) / 10;

        // Read light
        light = analogRead(BASE + 2);
        // Read humidity
        humidity = analogRead(BASE + 1);

        // Calculate v_out
        v_out = (light / 1023) * humidity;

        delay(interval * 1000);
    }
    pthread_exit(NULL);
}


int decCompensation(int units){

    int unitsU = units%10;

    if(units >= 50){

        units = 0x50 + unitsU;
    }

    else if(units >= 40){

        units = 0x40 + unitsU;
    }

    else if(units >= 30){

        units = 0x30 + unitsU;
    }

    else if(units >= 20){

        units = 0x20 + unitsU;
    }

    else if(units >= 10){

        units = 0x10 + unitsU;
    }

    return units;
}

int hexCompensation(int units){
    int unitsU = units%0x10;

    if(units >= 50){

        units = 50 + unitsU;
    }

    else if(units >= 40){

        units = 40 + unitsU;
    }

    else if(units >= 30){

        units = 30 + unitsU;
    }

    else if(units >= 20){

        units = 20 + unitsU;
    }

    else if(units >= 10){

        units = 10 + unitsU;
    }

    return units;


}

int hFormat(int hours){
    if(hours >= 24)
        hours = 0;
    else if(hours > 12)
        hours -= 12;

    return (int)hours;
}



void tick(void){
    //hours = hexCompensation(hours);
    //mins = hexCompensation(mins);
    //secs = hexCompensation(secs);

    secs+=1;

    if(secs >= 60){
        secs = 0;
        mins++;
    }

    if(mins >= 60){
        hours++;
        mins = 0;
    }

    hours = hFormat(hours);
}

void systemTime(void){
    seconds += 1;
    if(seconds >= 60){
        ++minutes;
        seconds = 0;
    }
    if(minutes >= 60){
        ++hour;
        minutes = 0;
    }
    hour = hFormat(hour);
}

///////////////////////////////////////////////////////////////////
//         FUNCTIONS FOR MANAGING MQTT COMMUNICATION
//                  WITH THE REMOTE BROKER
//////////////////////////////////////////////////////////////////

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
    int qos         = 0;
    int feedback1 = mosquitto_subscribe(mosq, mid, dissmiss, qos);
    int feedback2 = mosquitto_subscribe(mosq, mid, monitor, qos);

    switch(feedback1){
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
    switch(feedback2){
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
    int humidity_value = 23;
    char humid_payload[5];
    //char payload[50] = "Zibondiwe";
    //int payloadlen = strlen(humid_payload);

    //itoa(humid_payload, humidity_value, 10);
    sprintf(humid_payload, "%d", humidity_value);
    //char payload[50] = "Zibondiwe";
    int payloadlen = strlen(humid_payload);
    //char payload[50] = "Zibondiwe";
    //int payloadlen = strlen(humid_payload);

    int qos = 0;
    bool retain = false;
    int* mid = NULL;
    int feedback = mosquitto_publish(mosq, mid, dissmiss, payloadlen, humid_payload, qos, retain);
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
