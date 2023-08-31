#ifndef IPS_SO_H
#define IPS_SO_H

//Libraries
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <uapi/linux/wait.h>
#include <linux/semaphore.h>


//Constant definition
///Declaration of the registration device
#define REGISTRATION_DEVICE "registration"
///Declaration of the synchronous device
#define SYNCHRONOUS_DEVICE "synchronous"
///Declaration of the shared memory device
#define SHARED_MEM_DEVICE "shared_memory"


///Declaration of the size of the registration device
#define REGISTRATION_SIZE 1024
///Declaration of the size of the synchronous device
#define SYNCHRONOUS_SIZE 1024
///Declaration of the size of the shared memory device
#define SHARED_MEM_SIZE 4096

///Declaration of the semaphore sem;
///This semaphore ensures each Queue_mes got a unique arrival
static struct semaphore sem;


//Devices utilities:
///Unique device identifier assigned the registration device
static dev_t dev_num_registration;
///character device structure for the registration device. 
static struct cdev cdev_registration;
///Variable used to create the device node in the /dev directory
static struct class *cl_registration;

///Unique device identifier assigned the synchronous device
static dev_t dev_num_synchronous;
///character device structure for the synchronous device. 
static struct cdev cdev_synchronous;
///Variable used to create the device node in the /dev directory
static struct class *cl_synchronous;

///Unique device identifier assigned the shared memory device
static dev_t dev_num_shared;
///character device structure for the shared memory device. 
static struct cdev cdev_shared;
///Variable used to create the device node in the /dev directory
static struct class *cl_shared;


//Devices buffers:
///Pointer to the buffer registration.
///@see REGISTRATION_SIZE
static char *registration_buffer;
///Pointer to the buffer synchronous. 
///@see SYNCHRONOUS_SIZE
static char *synchronous_buffer;
///Pointer to the buffer shared memory. 
///@see SHARED_MEM_SIZE
static char *shared_buffer;


//Global: Sync
///This is the declaration of the type sync_pid
typedef struct sync_pid {
    int pid;                            //pid of the process
    ///variable that allows process to sleep
    wait_queue_head_t wait_queue;       //initial wait queue for the process
    ///variable that allows process to sleep for an amount of time
    wait_queue_head_t wait_queue_delay; //wait queue for the delay
    ///flag to check if the wait queue has been woken up
    bool letto;                         //flag to check if the wait queue has been woken up
    int delay;                          //delay of the process
    ///flag to check if the delay has been changed
    bool change_delay;                  //flag to check if the delay has been changed
    ///flag to check if this istance of struct sync_pid has been modified
    int first_time;                     //flag to check the wait_queue
    ///variable used as unique ID
    int arrival;                        //arrival time of the message (ID)
    ///represents the delay that the process has to wait
    int effective_delay;                //effective delay of the message
    struct sync_pid *next;              //pointer to the next element
} sync_pid;

///This is the declaration of the type changeData.
/// @brief this struct is used to change the data inside sync_pid if necessary
typedef struct changeData{
    ///variable used as unique ID
    int arrival;            //arrival time of the message (ID)
    int delay;              //delay of the process
    ///represents the delay that the process has to wait
    int effective_delay;    //effective delay of the message
} changeData;


///Pointer to the first sync_pid element in the queue.
sync_pid *first_sync = NULL;
///Pointer to the last sync_pid element in the queue.
sync_pid **last_sync = NULL;


//Global: Priority Queue
///This is the declaration of the type Message
typedef struct Message{
    int pid_recipient;    //recipient's PID    
    int pid_sender;       //sender's PID 
    ///Message's priority ranging from 1 to 10
    int priority;         //priority of the message
    ///Delay with which the message will be sent (expressed in seconds)
    int delay;            //delay of the message
    ///Message payload (max 64 alphanumeric symbols)
    char payload[128];    //payload in bytes
} Message;

///This is the declaration of the type Queue_mes
typedef struct Queue_mes{
    /// Nested field. @see Message for more info
    Message mes;             //nested field struct Message
    ///Arrival time of the message when enqueue into PQ[i]. 
    /// @see PriorityEnqueue_mes()
    unsigned long arrival;   //time of arrival
    ///Pointer to the next element
    struct Queue_mes *next;  //pointer to the next element
} Queue_mes;

///This is the declaration of the type Queue_buffer
typedef struct Queue_buffer{
    ///This field contains only messages with delay expired. 
    /// @see Message for more info
    Message mymes;              //buffer queue item
    ///Pointer to the next element
    struct Queue_buffer *next;  //pointer to the next element
} Queue_buffer;

///Array of pointers to the last element of each PQ[i]
/// @see Queue_mes **PQ
Queue_mes *pq_last[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}; //global 'last' label to the last pointer of each Queue_mes.
///Array of the 10 priority queues, one for each priority level
Queue_mes **PQ;
//Pointer to the first element of the queue Queue_buffer
Queue_buffer *head = NULL;


//Global: registration
///This is the declaration of the type queue_pid
typedef struct queue_pid{
    ///This field contains the PID of a subscriber to IPC-SO
    int pid;
    ///Pointer to the next element
    struct queue_pid *next;
} queue_pid;

///Pointer to the first element of the queue queue_pid
queue_pid *first = NULL;
///Pointer to the last element of the queue queue_pid
queue_pid **last = NULL;



//FUNCTIONS DECLARATION:


///Initialization routine of the module IPC-SO
///@returns 0, no errors occurs.
///@returns PTR_ERR, Failed to create device class
///@returns -ENOMEM, Failed to allocate buffer
///@returns ret < 0, Failed to allocate character device region
///@returns ret < 0, Failed to add character device 
///@returns PTR_ERR, PQ is not correctly initialized
static int __init ipc_os_module_init(void);
///Deinitialization routine of the module IPC-SO, destroy the devices and free the memory.
static void __exit ipc_os_module_exit(void); 


//Registration Functions
///Callback function, part of the file operation structure associated with the device.
///@param inode Represents the inode structure of the file being opened
///@param file Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully opened and ready.
static int registration_open(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device.
///@param inode, Represents the inode structure of the file being opened
///@param file, Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully closed.
static int registration_release(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, total number of bytes read and copied to the user buffer
///@return 0, bytes_to_copy is less than or equal to 0
///@return < 0, an error occurs during the copy_to_user operation
static ssize_t registration_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset);
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, Number of bytes consumed from the user buffer
///@return 0, bytes_to_copy is less than or equal to 0
///@return -EINVAL, Size of the remaining user buffer is less than the size of an integer
///@return -EFAULT, An error occurs during the copy_from_user operation
static ssize_t registration_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset);


//Shared memory functions
///Callback function, part of the file operation structure associated with the device.
///@param inode Represents the inode structure of the file being opened
///@param file Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully opened and ready.
static int shared_memory_open(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device.
///@param inode, Represents the inode structure of the file being opened
///@param file, Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully closed.
static int shared_memory_release(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, total number of bytes read and copied to the user buffer
///@return 0, remaining_size is zero or less, the function returns 0
///@return -EFAULT, an error occurs during the copy_to_user operation
static ssize_t shared_memory_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset);
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, Number of bytes consumed from the user buffer
///@return 0, bytes_to_copy is less than or equal to 0
///@return -ENOMEM, memory allocation for the user_message fails
///@return -EFAULT, An error occurs during the copy_from_user operation
///@return -EACCES, the current process is not running as root (euid is not 0) and it tries to send a message with priority 10
static ssize_t shared_memory_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset);


//Synchronous functions
///Callback function, part of the file operation structure associated with the device.
///@param inode Represents the inode structure of the file being opened
///@param file Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully opened and ready.
static int synchronous_open(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device.
///@param inode, Represents the inode structure of the file being opened
///@param file, Represents the file structure associated with the file being opened
///@returns 0, device file has been successfully closed.
static int synchronous_release(struct inode *inode, struct file *file);
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, the operation was successful, and the message was successfully read from the synchronous priority queue and copied to the user buffer
///@return 0,  bytes_this_message is zero or less, the function returns 0
///@return -EFAULT,  an error occurs during the copy_to_user operation
static ssize_t synchronous_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset); 
///Callback function, part of the file operation structure associated with the device. 
///@param file Pointer to the 'stuct file' representing the opened file
///@param user_buf Pointer to the user-space buffer where the data read from the device should be copied
///@param size Number of bytes that the user-space buffer can hold
///@param offset Pointer to the current file offset
///@return > 0, operation was successful, and the process has been successfully registered for synchronous communication
///@return 0, bytes_to_copy is zero or less
///@return -EFAULT, an error occurs during the copy_from_user operation
static ssize_t synchronous_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset);


//Sync functions
///Initialize a new node of type sync_pid
///@param id It's the PID of the process
///@returns ERR_PTR(-EFAULT), If kmalloc returns a NULL pointer, 
///@returns ttp, Valid pointer to the newly allocated queue_pid structure
sync_pid *sync_init(int id);
///Insert the new item sync_pid into the sync queue.
///@param first_sync Global pointer to the first element
///@param item New item of type sync_pid ready to be enqueued into the pid's queue.
void sync_insert(sync_pid **first_sync, sync_pid *item);
///Delete a target sync_pid item from the sync queue 
///@param first_sync Global pointer to the first element
///@param target target PID of the element that has to be dequeued.
void sync_delete(sync_pid **first_sync, int target);
///Deinit the entire pid's queue and free the memory
///@param first_sync Global pointer to the first element
void sync_deinit(sync_pid **first_sync);
///Scan the sync queue and determinate if a target PID is in it
///@param first_sync Global pointer to the first element
///@param val Target PID value for the research
///@returns found, sync_pid structure with the matching PID if found
///@returns NULL, no sync_pid structure with the given PID was found in the queue
sync_pid *sync_search(sync_pid **first_sync, int val);
///Dequeue a specific message from PQ using it's arrival
///@param target_arrival Target for the dequeue operation
///@return target_sync_mes, returns the dequeued target Message
Message PriorityDequeue_sync(int target_arrival);
///Compares different messages with the target pid and return the one with the minimum delay
///@param target_pid represents the target PID for which we want to find the delay-related data
///@return changeData, structure of type changeData
changeData sync_mes_find(int target_pid);
///Determine whether there are messages in the priority queues with a specific target PID
///@param var Represents the target PID that we want to check for in the messages
///@return 0, if any messages were not found
///@return 1, if any messages were found
int sync_mes_find_check(int var);



//Registration Queue_mes functions:
///Initialize a new node of type queue_pid
///@param id PID correlated to a new process subscribed to IPC-SO
///@returns ERR_PTR(-EFAULT), If kmalloc returns a NULL pointer, 
///@returns ttp, Valid pointer to the newly allocated queue_pid structure
queue_pid *reg_init(int id);
///Insert the new item queue_pid into the pid queue.
///@param first Global pointer to the first element
///@param item New item of type queue_pid ready to be enqueued into the pid's queue.
void reg_insert(queue_pid **first, queue_pid *item);
///Delete a target queue_pid item from the pid's queue 
///@param first Global pointer to the first element
///@param target target PID of the element that has to be dequeued.
void reg_delete(queue_pid **first, int target);
///Deinit the entire pid's queue and free the memory
/// @param first Global pointer to the first element
void reg_deinit(queue_pid **first);
///Search a specific PID into the pid's queue, used to discriminate between registration and unregistration operations
///@param first Global pointer to the first element
///@param val identifier of the target PID
///@return 0, The value is not found into the pid's queue => registration
///@return 1, The value is found into the pid's queue => unregistration
int reg_search(queue_pid **first, int val);


//PQ functions
///Initialize the vector of Queue_mes, called PQ (Priority Queue)
///@returns myPQ, valid pointer to an array of Queue_mes pointers
///@returns NULL, memory allocation for the array of Queue_mes pointers fails
///@returns ERR_PTR(-EFAULT), memory allocation for an individual priority queue within the array fails
Queue_mes **pqinit(void);
///Dequeue a specific Queue_mes from the respective PQ[i]
///@param first Global pointer to the first element
///@param pid_caller target PID that represents the nested Messages of interests
///@see Queue_mes and Message for mor details
void PriorityDequeue_mes(Queue_mes **first, int pid_caller);           
///Output information about the elements stored in the priority queues for debugging or informational purposes.
///@param t Global pointer to the first element
void pq_print(Queue_mes **t);                                //print all the 10 priority queues 
///Enqueue a new item Queue_mes in the respective PQ[i] level
/// @param t Global pointer to the first element
/// @param m Message from the user process
void PriorityEnqueue_mes(Queue_mes **t, Message *m);          //enqueue a new message
///Dequeue each PQ[i] and free the memory
/// @param t Global pointer to the first element
void pq_deinit(Queue_mes **t);                              //free the memory 



//Buffer Queue functions
///Initialize a new node of type Queue_buffer
///@param tmp temporary Message ready to be enqueued
///@returns new_node, memory allocation for the new Queue_buffer node is successful
///@returns ERR_PTR(-EFAULT), indicate the allocation failure
Queue_buffer *bq_init(Message tmp);
///Enqueue a new item of type Queue_buffer in the buffer queue
///@param tmp Item of type Queue_buffer, ready to be enqueued
void bq_enqueue(Queue_buffer *tmp);
///Dequeue the head item from the buffer queue
///@returns *msg, pointers to the first message in the buffer queue (FIFO)
///@returns NULL, if the buffer queue is empty
Message *bq_dequeue(void);


#endif