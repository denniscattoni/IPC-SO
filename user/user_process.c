#include "USER_PROCESS.h"


int main() {
    
    srand(time(NULL));

    int stop = 0;
    int operation;
    int reg=0;

    //Handle errors
    int err;
    
    reg=-1;
    err = reg_unreg(reg);
    reg=1;
    
    do{

        operation = mymenu(reg);
        
        if(reg==1){

        switch(operation){
        case 1:
            err = reg_unreg(reg);
            if(reg==0){
            	reg=1;
            }
            else if(reg==1){
            	reg=0;
            }
            myerr(err);
        break;

        case 2:
            err = process_avaiable();
            myerr(err);
        break;

        case 3:
            err = mes_write();
            myerr(err);
        break;

        case 4:
            err = mes_read();
            myerr(err);
        break;

        case 5:
            err = mes_read_sync();
            myerr(err);
        break;

        case 6:
            err = reg_unreg(reg);
            reg=0;
            myerr(err);
            stop=1;
        break;
        
        default:
            printf("Please, select a correct operation!\n");
            printf("Operation: %d", operation);
        break;
        }
        
        } else {

            switch(operation){
                case 1:
                    err = reg_unreg(reg);
                    if(reg==0){
                        reg=1;
                    } else if(reg==1){
                        reg=0;
                    }
                    myerr(err);
                    break;

                case 2:
                    reg=0;
                    myerr(err);
                    stop=1;
                break;

                default:
                    printf("Please, select a correct operation!\n");
                    printf("Operation: %d", operation);
                break;
            }
        }
        
    } while(!stop);

    return 0;
}



//Functions:----------------------------------------------------------------------------------

Message *newMessage(){
    
    int pid_dest;
    int pid;
    char payload[128];
    Message *tmp = (Message *)malloc(sizeof(Message));

    if (tmp == NULL) {
        exit(-1);
    }

    // Get the PID of the current process
    pid = getpid();

    if(AUTO==1){
        sprintf(payload, "Hello from Process %d!", pid);
        tmp->pid_recipient;
        tmp->pid_sender = pid;
        tmp->priority = myRandom(1,10);
        tmp->delay = 0; //set a 0 for debug, if you want a delay in **AUTO** mode modify here
    } else {
    	int pr=0;
    	int delay=0;
    	
    	tmp->pid_recipient;
    	tmp->pid_sender = pid;
    	
    	char msg[65];
    	printf("Insert your message (max 64 alphanumeric symbols): ");
    	getchar();
    	
    	//functions that truncate the paylod at 64 characters
        if(fgets(msg, sizeof(msg), stdin) != NULL) {

        //Remove the newline character if it exists
        size_t length = strlen(msg);
        if (length > 0 && msg[length - 1] == '\n') {
            msg[length - 1] = '\0';
        }
        else{
            //clear the buffer until the 64 character
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        //Now 'msg' contains a properly handled string
        printf("You entered: %s\n", msg);

    } else {
        printf("Error reading input.\n");
    }
    	
    	//verify that priority is >=1 or <=10
    	do{
    	    printf("Insert the message priority : ");
    	    scanf("%d",&pr);
    	}while(pr<1 || pr>10);
    	tmp->priority = pr;
    	
    	//verify that the delay is >=0
    	do{
    	    printf("Insert the delay : ");
    	    scanf("%d",&delay);
    	}while(delay<0);
    	tmp->delay = delay;
    }

    //Hex conversion
    int messageLength = strlen(payload);
    int hexBytesLength = 128;
    char hexBytes[hexBytesLength];

    //Convert payload to hex and copy to hexBytes
    for(int i = 0; i < messageLength; i++){
        snprintf(&hexBytes[i * 2], hexBytesLength - i * 2, "%02X", (unsigned char)payload[i]);
    }
    hexBytes[messageLength * 2] = '\0';    //Null-terminate the hex string

    //Copy the hex string into the payload array
    strcpy(tmp->payload, hexBytes);

    return tmp;
}

int myRandom(int min, int max){
    int random = rand() % (max - min + 1) + min;
    return(random);
}

int mymenu(int reg){
    int pid = getpid();
    int op = -1;

    printf("\n-------------------------------------------------\n");  
    if(reg==1){      
	    printf("Current process PID: %d\n",pid); 
	    printf("\n1, unregister the current process");
	    printf("\n2, list of avaiable processes");
	    printf("\n3, write a message");
	    printf("\n4, read a message asynchronous mode"); 
	    printf("\n5, read a message synchronous mode");
	    printf("\n6, exit");
	    printf("\nWhich operation do you want to do?  -->");
    }
    else
    {
    	printf("\n1, register the current process");
    	printf("\n2, exit");
    	printf("\nWhich operation do you want to do?  -->");
    }
    scanf("%d", &op);
    printf("\n");

    return(op);
}

int reg_unreg(int reg){

    int fd = open(REGISTRATION_DEVICE, O_RDWR);
    
    if (fd < 0) {
        return 1;
    }

    // Get the PID of the current process
    int pid = getpid();
    
    if(reg==-1){
    	printf("Welcome to IPC-SO, process: %d\n", pid);
    }
    

    //Write the PID as an integer to the registration device
    ssize_t bytes_written = write(fd, &pid, sizeof(int)); // Use sizeof(int) instead of sizeof(pid)
    if (bytes_written < 0) {
        close(fd);
        return 2;
    }

    close(fd);

    return 0;
}

int process_avaiable(){
    
    //Open the registration device for reading
    int fd = open(REGISTRATION_DEVICE, O_RDWR);
    if (fd < 0) {
        return 3;
    }

    int pid_received[MAX_REGISTRATIONS]; //Assuming MAX_REGISTRATIONS as the maximum number of PIDs to read from the buffer
    ssize_t bytes_read = read(fd, &pid_received, sizeof(pid_received));
    
    if (bytes_read < 0) {
        close(fd);
        return 4;
    }

    //Calculate the number of PIDs received
    int num_pids_received = bytes_read / sizeof(int);

    //Print the received PIDs as integers
    printf("Avaiable %d PIDs:\n", num_pids_received);
    
    for(int i = 0; i < num_pids_received; i++){
        printf("Received PID %d: %d\n", i + 1, pid_received[i]);
    }

    close(fd); 

    return 0;
}

bool check(int pid){
    //Open the registration device for reading
    int fd = open(REGISTRATION_DEVICE, O_RDWR);
    if (fd < 0) {
        return false;
    }

    int pid_received[MAX_REGISTRATIONS]; //Assuming MAX_REGISTRATIONS as the maximum number of PIDs to read from the buffer
    ssize_t bytes_read = read(fd, &pid_received, sizeof(pid_received));
    
    if (bytes_read < 0) {
        close(fd);
        return false;
    }

    //Calculate the number of PIDs received
    int num_pids_received = bytes_read / sizeof(int);
    
    bool rtn=false;
    for(int i = 0; i < num_pids_received; i++){
        if(pid_received[i]==pid){
        	rtn=true;
        }
       
    }

    close(fd); 

    if(!rtn){
    	printf("Incorrect process PID! Insert a correct PID. \n");
    }
    
    return rtn;
}

int mes_write() {
    int fd = open(SHARED_MEM_DEVICE, O_WRONLY);

    if(fd < 0){
        return 5;
    }

    Message *my_message = newMessage();
    ssize_t bytes_written;
    int choice = -1;
    
    do {
        int new_pid_dest;
        
        //verirfy that the recipient's PID exist  
        do{
        	printf("Enter the recipient's PID: ");
        	scanf("%d", &new_pid_dest);
        }while(!check(new_pid_dest));
         
        my_message->pid_recipient = new_pid_dest;

        bytes_written = write(fd, my_message, sizeof(Message));
        if(bytes_written < 0){
            close(fd);
            free(my_message);
            return 6;
        }

        printf("Do you want to send the same message to another recipient? (1/0) --> ");
        scanf("%d", &choice);
        


    } while(choice == 1);

    free(my_message);
    close(fd);

    return 0;
}

int mes_read() {
    Message received_message;
    ssize_t bytes_read;
    int pid = (int)getpid();
    int messages_received = 0;
    int byte;

    int fd = open(SHARED_MEM_DEVICE, O_RDWR);

    if (fd < 0) {
        return 7;
    }

    //Loop to read and print all messages
    while((bytes_read = read(fd, &received_message, sizeof(Message))) > 0){
        
        if (received_message.pid_recipient == pid) {
            printf("\nReceived message %d:\n", messages_received + 1);
            printf("Recipient PID: %d\n", received_message.pid_recipient);
            printf("Sender PID: %d\n", received_message.pid_sender);
            printf("Priority: %d\n", received_message.priority);
            printf("Delay: %d\n", received_message.delay);
            printf("Payload: ");

            // Convert payload bytes to string
            for (ssize_t i = 0; i < bytes_read; i += 2) {
                if (sscanf((char *)&received_message.payload[i], "%02X", &byte) != 1 || byte == 0) {
                    if (byte == 0) {
                        // Null terminator found
                        printf("\n");
                    }
                    break;
                }
                printf("%c", (char)byte);
            } 

            printf("\n");

            messages_received++;
        }
    }

    if(messages_received == 0){
        printf("\nNo messages available for this process!\n");
    }

    close(fd);

    return 0;
}

int mes_read_sync() {
    Message received_message;
    Message received_message_fake;
    ssize_t bytes_read;
    int pid = (int)getpid();
    int messages_received = 0;
    int byte;    
    
    int fd_sync=open(SYNCHRONOUS_DEVICE, O_RDWR);

    if(fd_sync < 0){
        perror("Error opening synchronous device");
        return -1;
    }

    //Write the PID as an integer to the registration device
    ssize_t bytes_written = write(fd_sync, &pid, sizeof(int));
    if(bytes_written < 0){
        perror("Error writing PID to the registration device");
        close(fd_sync);
        return -1;
    }

    read(fd_sync, &received_message, sizeof(Message));

    if(received_message.pid_recipient == pid){
        printf("\nReceived message %d:\n", messages_received + 1);
        printf("Recipient PID: %d\n", received_message.pid_recipient);
        printf("Sender PID: %d\n", received_message.pid_sender);
        printf("Priority: %d\n", received_message.priority);
        printf("Delay: %d\n", received_message.delay);
        printf("Payload: ");
        
        //Convert payload bytes to string
        for(ssize_t i = 0; i < bytes_read; i += 2){

            if(sscanf((char *)&received_message.payload[i], "%02X", &byte) != 1 || byte == 0) {
                if (byte == 0) {
                    //Null terminator found
                    printf("\n");
                }
                break;
            }
            printf("%c", (char)byte);
        }

        printf("\n");
        messages_received++;
    }

    if(messages_received == 0){
        printf("\nNo messages available for this process!\n");
    }

    close(fd_sync);

    return 0;
}

void myerr(int er){
    switch(er){
        case -1:
         	perror("Error: memory for new Message not allocated.");
        break;
         
        case 1:
         	perror("Error: opening registration device");
        break;
         
        case 2:
         	perror("Error: writing PID to the registration device");
        break;
         
        case 3:
         	perror("Error: opening registration device");
        break;
         
        case 4:
         	perror("Error: reading PIDs from the kernel module");
        break;
         
        case 5:
         	perror("Error: opening shared memory device");
        break;
         
        case 6:
         	perror("Error: writing message to shared memory");
        break;
         
        case 7:
         	perror("Error: opening shared memory device");
        break;
         
        case 8:
         	perror("Error: opening shared memory device");
        break;
         
        case 9:
         	perror("Error: opening synchronous device");
        break;
         
        case 10:
         	perror("Error: writing PID to the registration device");
        break;
    }

}










