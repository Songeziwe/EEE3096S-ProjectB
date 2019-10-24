#ifndef _PROJECT
#define _PROJECT

// Includes
#include <iostream>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <pthread.h>
#include <sched.h>
#include "CurrentTime.h"

// For MQTT
#include <mosquitto.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>

// MACROS
#define BASE 100
#define SPI_CHAN 0
#define SPI_SPEED 1350000

// For online MQTT broker
#define HOST "m14.cloudmqtt.com"
#define PORT 16797
#define KEEPALIVE 60
#define USERNAME "xkxabjtv"
#define PASSWORD "aY6G1txTZr-q"

struct mosquitto* mosq;
char topic[50] = "testTopic"

// Functions to manage communication with the remote broker
int setup(void);
int subscribe(void);
int publish(void);
void on_message_callback(struct mosquitto *, void *, const struct mosquitto_message*);

const int BTNS[] = {7, 0, 2, 3};

// RTC
const char RTCAddr  = 0x6f;
const char SEC      = 0x00;
const char MIN      = 0x01;
const char HOUR     = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

// Declare interrupt handlers
void start_stop_monitoring(void);
int initGPIO(void);
void dissmiss_alarm(void);
void reset_system_time(void);
void change_reading_interval(void);
void tick(void);

int decCompensation(int hours);
int hexCompensation(int x);
int hFormat(int hours);

void *readFromADC(void *threadargs);

void systemTime(void);
void lightLED(int units);
#endif
