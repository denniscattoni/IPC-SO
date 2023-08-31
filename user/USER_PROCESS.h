#ifndef USER_PROCESS_H
#define USER_PROCESS_H

//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h> 
#include <string.h>
#include <time.h>
#include <stdbool.h>


//Type definition
///Message structure
typedef struct Message{
    int pid_recipient;    //recipient's PID    
    int pid_sender;       //sender's PID 
    ///Message priority ranging from 1 to 10
    int priority;         //priority of the message
    ///Delay with which the message will be sent (expressed in seconds)
    int delay;            //delay of the message
    ///Message payload (max 64 alphanumeric symbols)
    char payload[128];    //payload in bytes
} Message;


//Constant definition
///Defines a debug variable to auto-initialized struct Message
#define AUTO 0
#define REGISTRATION_DEVICE "/dev/registration"
#define SHARED_MEM_DEVICE "/dev/shared_memory"
#define SYNCHRONOUS_DEVICE "/dev/synchronous"
///Defines the maximum number of PIDs that can be written into the registration buffer
#define MAX_REGISTRATIONS 256


//Function declaration
///Function that allows you to create a new message
///@returns tmp Returns the structure of the message with all fields
Message *newMessage();

///Function that chooses a random number between min and max
///@param min Lower Limit
///@param max Upper Limit
int myRandom(int min, int max);

///Function that prints the menu to choose the operation to carry out
///@param reg Indicates whether the process is already registered in the kernel
///@returns op Returns the operation that was selected
int mymenu(int reg);

///Function that allows the registration/delete of the process inside the kernel
///@param reg Indicates whether the process is already registered in the kernel
///@returns 0 -> If there are no errors
///@returns 1 -> If the process fails the opening of the registration device
///@returns 2 -> If the process fails write to the registration device
int reg_unreg(int reg);

///Function that allows you to view all the processes registered within the kernel
///@returns 0 -> If there are no errors
///@returns 3 -> If the process fails to open the shared memory
///@returns 4 -> If the process fails read the pid from the kernel
int process_avaiable();

///Function that sends the message to a recipient chosen by the user
///@returns 0 -> If there are no errors
///@returns 5 -> If the process fails to open the shared memory
///@returns 6 -> If the process fails to write to shared memory
int mes_write();

///Function used to read incoming messages
///@returns 0 -> If there are no errors
///@returns 7 -> If the process fails to open the shared memory
int mes_read();

///Function used to read incoming messages (synchronously)
///@returns 0 -> If there are no errors
///@returns 8 -> If the process fails to open the shared memory
///@returns 9 -> If the process fails the opening of synchronous processes
///@returns 10 -> If the process does not write the pid in the registration device
int mes_read_sync();

///Function that prints the description of the errors within the process
void myerr(int er);

///Function that checks if the entered pid is present in the list of processes registered in the kernel
///@param pid Indicates the PID to be searched
///@returns true If it finds the PID you are looking for
///@returns false If it does not find the PID you are looking for
///@see Use the process_avaiable() function to check process PIDs
bool check(int pid); 

#endif
