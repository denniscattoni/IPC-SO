#include "IPC_SO.h"


//FUNCTIONS:


//Registration ###################################

static int registration_open(struct inode *inode, struct file *file) {
    return 0;
}

static int registration_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t registration_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset) {
    int num_pids = 0;
    int bytes_to_copy = min_t(size_t, size, (size_t)(REGISTRATION_SIZE - *offset));
    int *dest = (int *)user_buf;
    queue_pid *curr;

    if(bytes_to_copy <= 0){
        return 0;
    }

    //Set the current position in the Queue_mes
    curr = first;

    //Traverse the Queue_mes and send all integer PIDs to the user buffer
    while(curr && num_pids < (bytes_to_copy / sizeof(int))){
        
        if(copy_to_user(dest, &(curr->pid), sizeof(int))){
            return -EFAULT;
        }

        dest++;
        num_pids++;
        curr = curr->next;
    }

    *offset += num_pids * sizeof(int);
    return num_pids * sizeof(int);
}

static ssize_t registration_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset) {
    int bytes_to_copy = min_t(size_t, size, (size_t)REGISTRATION_SIZE - *offset);
    int value;
    int found;
    queue_pid *tmp;
    
    if(bytes_to_copy <= 0){
        return 0;
    }

    //Ensure the buffer is large enough to store an integer
    if(bytes_to_copy < sizeof(int)){
        return -EINVAL; 
    }

    //Extract the integer value from the user buffer
    if(copy_from_user(&value, user_buf, sizeof(int))){
        return -EFAULT;
    }

    //Move the user buffer pointer forward to skip the integer data
    user_buf += sizeof(int);
    bytes_to_copy -= sizeof(int);
    *offset += sizeof(int);

    found = reg_search(&first, value);

    //if the value is found, I know it's an unregister operation
    if(found == 0){

        //making the PID's Queue_mes:
        tmp = reg_init(value);
        reg_insert(&first, tmp);

    } else {
        reg_delete(&first, value);
    }

    return bytes_to_copy;
}



//Synchronous ####################################

static int synchronous_open(struct inode *inode, struct file *file) {
    return 0;
}

static int synchronous_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t synchronous_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset) {
    int mess_process=0;  //Used to know if there's a message for this process
    changeData box;  //Used to store the data that we need to change in the sync_pid struct
    int arrival;  //Used as an unique ID for the message
    int caller_pid = current->pid; //Used to know the PID of the process that is calling the read
    sync_pid *found;  //used to check which process of the sync_pid list is the one that is calling the read
    unsigned long timeout_jiffies; //Used to set the timeout to the wait_event_interruptible_timeout
    int delay;  //Used to store the delay of the process that is calling the read
    int bytes_to_copy = min_t(size_t, size, (size_t)SHARED_MEM_SIZE - *offset);
    int remaining_size = bytes_to_copy;
    int copied_bytes = 0;
    Message tmp; 
    int message_size;
    int bytes_this_message;
    bool timer_expired = false;  //Used to check if the timer expired
    long result;  //I use it to check the result of the wait_event_interruptible_timeout

    

    found = sync_search(&first_sync, caller_pid); //Used to check which process of the sync_pid list is the one that is calling the read
    mess_process=sync_mes_find_check(caller_pid); //Used to know if there's a message for this process

    if(found != NULL){
        //if mess_process == 0 it means that there's no message for this process
        if(mess_process == 0){
            wait_event_interruptible(found->wait_queue, found->letto); //I wait until there's a message for this process
            found->letto=false; //I set the value of letto to false because I read the message
            found->change_delay=false; //I set the value of change_delay to false because I read the message
            //the while is used to beeing sure the delay of the message is expired, thats why the timer_expired is set to true only if the timer expired
            
            while(timer_expired == false){
               
                //I calculate the delay in jiffies and I set the timeout to the wait_event_interruptible_timeout
                delay=found->effective_delay*1000;
                timeout_jiffies=msecs_to_jiffies(delay);
                result = wait_event_interruptible_timeout(found->wait_queue_delay, found->change_delay, timeout_jiffies);
                
                if(result == 0){
                    //means the timer expired
                    timer_expired=true;
                    found->change_delay=true;
                } else if (result > 0) {
                    //means another process woke me up
                    found->change_delay=false;
                } else {
                    //something went wrong
                    found->change_delay=false;
                }
            }
        }else{
            //if mess_process != 0 it means that there's a message for this process
            //I make sure if it's the first time that the process is calling the read
            if(found->first_time == 0){
                box=sync_mes_find(caller_pid);
                found->arrival=box.arrival;
                found->delay=box.delay;
                found->effective_delay=box.effective_delay;
                found->first_time=1;
            }
            //used as the while above
            while(timer_expired == false){
                delay=found->effective_delay*1000;
                timeout_jiffies=msecs_to_jiffies(delay);
                result = wait_event_interruptible_timeout(found->wait_queue_delay, found->change_delay, timeout_jiffies);
                if (result == 0) {
                    timer_expired=true;
                    found->change_delay=true;
                } else if (result > 0) {
                    found->change_delay=false;
                } else {
                    found->change_delay=false;
                }
            }
        }
        arrival=found->arrival;
        sync_delete(&first_sync, caller_pid); //I delete the process from the sync_pid list
    }

    tmp=PriorityDequeue_sync(arrival); //I dequeue the message from the priority queue using arrival as an unique ID
    message_size = sizeof(Message);
    bytes_this_message = min_t(int, remaining_size, message_size);

    //Copy the message to user space
    if(copy_to_user(user_buf + copied_bytes, &tmp, bytes_this_message)){
        return -EFAULT;
    }

    // Update offsets and counters
    *offset += bytes_this_message;
    copied_bytes += bytes_this_message;
    remaining_size -= bytes_this_message;

    return copied_bytes;
}

static ssize_t synchronous_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset) {
    int bytes_to_copy = min_t(size_t, size, (size_t)SHARED_MEM_SIZE - *offset);
    int value;
    sync_pid *tmp;

    if (bytes_to_copy <= 0) {
        return 0;
    }

    if (copy_from_user(&value, user_buf, sizeof(value))) {
        return -EFAULT;
    }

    tmp=sync_init(value); //I initialize the process that is calling the write
    sync_insert(&first_sync, tmp); //I insert the process in the sync_pid list

    *offset += bytes_to_copy;
    return bytes_to_copy;
}



//Shared memory ##################################

static int shared_memory_open(struct inode *inode, struct file *file) {
    return 0;
}

static int shared_memory_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t shared_memory_read(struct file *file, char __user *user_buf, size_t size, loff_t *offset) {
    int bytes_to_copy = min_t(size_t, size, (size_t)SHARED_MEM_SIZE - *offset);
    int remaining_size = bytes_to_copy;
    int copied_bytes = 0;
    Message *tmp;
    int message_size;
    int bytes_this_message;
    int caller_pid = current->pid;

    //Dequeue messages from PQ[i] and enqueue into Queue_buffer
    PriorityDequeue_mes(PQ, caller_pid);

    while(remaining_size > 0){
        
        //Dequeue a message from Queue_buffer
        tmp = bq_dequeue();

        if(tmp == NULL){
            //No more messages in Queue_buffer
            break;
        }

        //Calculate the number of bytes to copy for this message
        message_size = sizeof(Message);
        bytes_this_message = min_t(int, remaining_size, message_size);

        //Copy the message to user space
        if(copy_to_user(user_buf + copied_bytes, tmp, bytes_this_message)){
            kfree(tmp); //deallocate tmp
            return -EFAULT;
        }

        //Update offsets and counters
        *offset += bytes_this_message;
        copied_bytes += bytes_this_message;
        remaining_size -= bytes_this_message;
        
        // Deallocate the message after it's been successfully copied
        kfree(tmp);
    }

    return copied_bytes;
}

static ssize_t shared_memory_write(struct file *file, const char __user *user_buf, size_t size, loff_t *offset) {
    
    ssize_t bytes_to_copy = min_t(size_t, size, (size_t)SHARED_MEM_SIZE - *offset);
    int priority_index;
    Message *user_message;
    sync_pid *found; //used to know if the process is in the sync_pid list
    changeData box; //used to store the data that are needed to change the process timeout

    //delay utilities
    unsigned int current_arrival;
    ktime_t current_time;
    s64 ns_since_boot;
    int time_since_enqueue_found;
    int time_before_left_found;

    //Root 10 priority handler 
    uid_t euid = current->cred->euid.val;
    pid_t pid = current->pid;


    user_message = (Message *)kmalloc(sizeof(Message), GFP_KERNEL);

    if(user_message == NULL){
        pr_err("Error: memory in shared_memory_write() not allocated.");
        return -ENOMEM;
    }

    if(bytes_to_copy <= 0){
        kfree(user_message); 
        return 0;
    }

    //Copy the message from the user space buffer to the allocated memory
    if(copy_from_user(user_message, user_buf, sizeof(Message))){
        kfree(user_message);
        return -EFAULT;
    }
    
    //The process is not running as root, reject the write operation
    if(euid != 0 && user_message->priority == 10){
        printk(KERN_ERR "Error: the process %d, is not allowed to send messages of priority 10.\n", (int)pid);
        return -EACCES;
    }
    
    //Calculate the priority queue index (in order to fits with array's index)
    priority_index = user_message->priority - 1;


    //Enqueue the message into the corresponding priority queue:
    down(&sem); //semaphore to ensure the arrival is an unique ID
        PriorityEnqueue_mes(PQ, user_message);
    up(&sem);

    found = sync_search(&first_sync, user_message->pid_recipient); //Used to knwo if the process is in the sync_pid list
    
    if(found != NULL){
        box=sync_mes_find(user_message->pid_recipient); //Used to know the data that are needed to change the process timeout
        //if first time==0 means the process needs to be woken up for the first time
        if(found->first_time==0){
            wake_up_interruptible(&found->wait_queue); //I wake up the process
            found->first_time++;
            found->letto=true;
            found->delay=user_message->delay;
            found->arrival=box.arrival;
            found->effective_delay=box.effective_delay;
        } else {

            //calculate and confront the old message effective delay with the new one
            current_time = ktime_get();
            ns_since_boot = ktime_to_ns(current_time);
            current_arrival = ns_since_boot / 1000000000ULL;
            time_since_enqueue_found = current_arrival - found->arrival;
            time_before_left_found = found->delay - time_since_enqueue_found;

            //if the new message has a lower effective delay than the old one, change the delay
            if (box.effective_delay<time_before_left_found){
                printk(KERN_INFO "Delay changed from %d to %d\n", time_before_left_found, box.effective_delay);
                found->delay=box.delay;
                found->effective_delay=box.effective_delay;
                found->change_delay=true;
                found->arrival=box.arrival;
                wake_up_interruptible(&found->wait_queue_delay); //I wake the process waiting and set the new timer on the read function
            }else{
                printk(KERN_INFO "Delay didnt change\n");
            }
        }
    }

    //Update the offset
    *offset += sizeof(Message);

    //free the memory of the Message
    kfree(user_message);
    
    return sizeof(Message);
}



//File operations-----------------------------------------------------------------------------

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = registration_open,
    .release = registration_release,
    .read = registration_read,
    .write = registration_write,
};

static struct file_operations fops_sync = {
    .owner = THIS_MODULE,
    .open = synchronous_open,
    .release = synchronous_release,
    .read = synchronous_read,
    .write = synchronous_write,
};

static struct file_operations fops_sm = {
    .owner = THIS_MODULE,
    .open = shared_memory_open,
    .release = shared_memory_release,
    .read = shared_memory_read,
    .write = shared_memory_write,
};

//--------------------------------------------------------------------------------------------



//IPC-OS init and exit:  ################################

static int __init ipc_os_module_init(void) {
    int ret = 0;
    
    //Allocate character device region for shared memory
    ret = alloc_chrdev_region(&dev_num_shared, 0, 1, SHARED_MEM_DEVICE);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate character device region for shared memory\n");
        return ret;
    }

    //Initialize and add shared memory cdev
    cdev_init(&cdev_shared, &fops_sm); 
    ret = cdev_add(&cdev_shared, dev_num_shared, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to add character device for shared memory\n");
        return ret;
    }

    //Create device class for shared memory
    cl_shared = class_create(THIS_MODULE, SHARED_MEM_DEVICE);
    if (IS_ERR(cl_shared)) {
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to create device class for shared memory\n");
        return PTR_ERR(cl_shared);
    }

    //Create shared memory device node
    device_create(cl_shared, NULL, dev_num_shared, NULL, SHARED_MEM_DEVICE);

    //Allocate memory for shared buffer
    shared_buffer = kmalloc(SHARED_MEM_SIZE, GFP_KERNEL);
    if (!shared_buffer) {
        device_destroy(cl_shared, dev_num_shared);
        class_destroy(cl_shared);
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to allocate shared buffer\n");
        return -ENOMEM;
    }

    //Allocate character device region for registration
    ret = alloc_chrdev_region(&dev_num_registration, 0, 1, REGISTRATION_DEVICE);
    if (ret < 0) {
        kfree(shared_buffer);
        device_destroy(cl_shared, dev_num_shared);
        class_destroy(cl_shared);
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to allocate character device region for registration\n");
        return ret;
    }

    //Initialize and add registration cdev
    cdev_init(&cdev_registration, &fops); 
    ret = cdev_add(&cdev_registration, dev_num_registration, 1);
    if (ret < 0) {
        kfree(shared_buffer);
        unregister_chrdev_region(dev_num_registration, 1);
        device_destroy(cl_shared, dev_num_shared);
        class_destroy(cl_shared);
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to add character device for registration\n");
        return ret;
    }

    //Create device class for registration
    cl_registration = class_create(THIS_MODULE, REGISTRATION_DEVICE);
    if (IS_ERR(cl_registration)) {
        cdev_del(&cdev_registration);
        kfree(shared_buffer);
        unregister_chrdev_region(dev_num_registration, 1);
        device_destroy(cl_shared, dev_num_shared);
        class_destroy(cl_shared);
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to create device class for registration\n");
        return PTR_ERR(cl_registration);
    }

    //Create registration device node
    device_create(cl_registration, NULL, dev_num_registration, NULL, REGISTRATION_DEVICE);

    //Allocate memory for registration buffer
    registration_buffer = kmalloc(REGISTRATION_SIZE, GFP_KERNEL);
    if (!registration_buffer) {
        device_destroy(cl_registration, dev_num_registration);
        class_destroy(cl_registration);
        cdev_del(&cdev_registration);
        kfree(shared_buffer);
        unregister_chrdev_region(dev_num_registration, 1);
        device_destroy(cl_shared, dev_num_shared);
        class_destroy(cl_shared);
        cdev_del(&cdev_shared);
        unregister_chrdev_region(dev_num_shared, 1);
        printk(KERN_ALERT "Failed to allocate registration buffer\n");
        return -ENOMEM;
    }


    //Allocate character device region for synchronous memory
    ret = alloc_chrdev_region(&dev_num_synchronous, 0, 1, SYNCHRONOUS_DEVICE);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate character device region\n");
        return ret;
    }

    //Initialize and add synchronous cdev
    cdev_init(&cdev_synchronous, &fops_sync);
    ret = cdev_add(&cdev_synchronous, dev_num_synchronous, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num_synchronous, 1);
        printk(KERN_ALERT "Failed to add character device\n");
        return ret;
    }

    //Create device class for synchronous
    cl_synchronous = class_create(THIS_MODULE, SYNCHRONOUS_DEVICE);
    if (IS_ERR(cl_synchronous)) {
        cdev_del(&cdev_synchronous);
        unregister_chrdev_region(dev_num_synchronous, 1);
        printk(KERN_ALERT "Failed to create device class\n");
        return PTR_ERR(cl_synchronous);
    }

    //Create synchronous device node
    device_create(cl_synchronous, NULL, dev_num_synchronous, NULL, SYNCHRONOUS_DEVICE);

    //Allocate memory for shared buffer
    synchronous_buffer = kmalloc(SYNCHRONOUS_SIZE, GFP_KERNEL);
    if (!synchronous_buffer) {
        device_destroy(cl_synchronous, dev_num_synchronous);
        class_destroy(cl_synchronous);
        cdev_del(&cdev_synchronous);
        unregister_chrdev_region(dev_num_synchronous, 1);
        printk(KERN_ALERT "Failed to allocate synchronous buffer\n");
        return -ENOMEM;
    }


    //Initialize the priority queue array
    PQ = pqinit();
    if(IS_ERR(PQ)){
        return PTR_ERR(PQ);
    }
    
    //initialize the semaphore
    sema_init(&sem, 1);

    printk(KERN_INFO "IPC-SO module initialized\n");
    return 0;
}

static void __exit ipc_os_module_exit(void) {
    //Deinitialize and free resources
    reg_deinit(&first);
    pq_deinit(PQ);
    sync_deinit(&first_sync);

    //Deallocate shared buffer memory
    kfree(shared_buffer);
    
    //Destroy shared memory device class and node
    device_destroy(cl_shared, dev_num_shared);
    class_destroy(cl_shared);
    
    //Remove and unregister shared memory cdev
    cdev_del(&cdev_shared);
    unregister_chrdev_region(dev_num_shared, 1);
    
    //Destroy registration device class and node
    device_destroy(cl_registration, dev_num_registration);
    class_destroy(cl_registration);
    
    //Remove and unregister registration cdev
    cdev_del(&cdev_registration);
    unregister_chrdev_region(dev_num_registration, 1);

    //Deallocate synchronous buffer memory
    kfree(synchronous_buffer);
    
    //Destroy synchronous device class and node
    device_destroy(cl_synchronous, dev_num_synchronous);
    class_destroy(cl_synchronous);
    
    //Remove and unregister synchronous cdev
    cdev_del(&cdev_synchronous);
    unregister_chrdev_region(dev_num_synchronous, 1);

    printk(KERN_INFO "IPC-SO module unloaded correctly.\n");
}



//Registration Queue_mes functions: ##################################

queue_pid *reg_init(int id) {
    queue_pid *ttp = kmalloc(sizeof(queue_pid), GFP_KERNEL);
    
    if(ttp == NULL){
        printk(KERN_ERR "Error: memory in reg_init() not allocated.\n");
        return ERR_PTR(-EFAULT);
    }

    ttp->pid = id;
    ttp->next = NULL;

    return ttp;
}

void reg_insert(queue_pid **first, queue_pid *item) {

    if(*first == NULL){
        *first = item;
        last = &((*first)->next);
    } else {
        *last = item;
        last = &((*last)->next);
    }
}

void reg_delete(queue_pid **first, int target) {
    queue_pid *tmp = *first;
    queue_pid *prev = NULL;

    //Target is the first
    if(tmp != NULL && tmp->pid == target){
        *first = tmp->next;
        kfree(tmp);
        first = NULL;
        return;
    }

    //Find the target in the middle or last
    while(tmp != NULL && tmp->pid != target){
        prev = tmp;
        tmp = tmp->next;
    }

    //Target not found
    if(tmp == NULL){
        printk(KERN_INFO "Target not found in the list reg_delete().\n");
        return;
    }

    //Target is the last
    if(tmp->next == NULL){
        prev->next = NULL;
        kfree(tmp);
        last = &prev->next; //Update last pointer to the new last element
    
    //Target is in the middle
    } else { 
        prev->next = tmp->next;
        kfree(tmp);
    }
}

void reg_deinit(queue_pid **first) {
    queue_pid *tmp;
    queue_pid *index;

    //empty pid's list
    if(*first == NULL){
        printk(KERN_INFO "\nList Queue_pid is already empty. No memory deallocation needed.");
        return;
    }

    index = *first;
    tmp = NULL;

    //deallocate all the pid's queue.
    while(index != NULL){
        tmp = index;
        index = index->next;
        kfree(tmp);
    }

    *first = NULL;
}

int reg_search(queue_pid **first, int val){
    int found = 0;
    queue_pid *tmp;

    if(first == NULL){
        printk(KERN_INFO "\nAttention! the pid's Queue is empty\n");
    } else {
       tmp = *first;

        while(tmp != NULL && !found){

            //found the target pid
            if(val == tmp->pid){
                found = 1;
            }
            tmp = tmp->next;
        }
    }

    return(found);
}



//Synchronous queue functions: ##################################

sync_pid *sync_init(int sync_pid_val) {
    sync_pid *item = kmalloc(sizeof(sync_pid), GFP_KERNEL);
    
    if (item == NULL) {
        printk(KERN_ERR "Error: memory in sync_init() not allocated.\n");
        return NULL;
    }

    item->pid = sync_pid_val;
    init_waitqueue_head(&item->wait_queue);
    init_waitqueue_head(&item->wait_queue_delay);
    item->first_time=0;
    item->letto = false;
    item->next = NULL;

    return item;
}

void sync_insert(sync_pid **first, sync_pid *item) {

    if (*first == NULL) {
        *first = item;
        last_sync = &((*first)->next);
    } else {
        *last_sync = item;
        last_sync = &((*last_sync)->next);
    }
}

void sync_delete(sync_pid **first, int target) {
    sync_pid *tmp = *first;
    sync_pid *prev = NULL;

    if(tmp != NULL && tmp->pid == target){
        *first = tmp->next;
        kfree(tmp);
        first = NULL;
        return;
    }

    while(tmp != NULL && tmp->pid != target){
        prev = tmp;
        tmp = tmp->next;
    }

    if(tmp == NULL){
        printk(KERN_INFO "Target not found in the list. sync_delete()\n");
        return;
    }

    if(tmp->next == NULL){
        prev->next = NULL;
        kfree(tmp);
        last_sync = &prev->next;
    } else {
        prev->next = tmp->next;
        kfree(tmp);
    }
}

void sync_deinit(sync_pid **first) {
    sync_pid *tmp;
    sync_pid *index;

    if (*first == NULL) {
        printk(KERN_INFO "\nList is already empty. No memory deallocation needed. sync_deinit()");
        return;
    }

    index = *first;
    tmp = NULL;

    while (index != NULL) {
        tmp = index;
        index = index->next;
        kfree(tmp);
    }

    *first = NULL;
    printk(KERN_INFO "\nMemory deallocated correctly. sync_deinit()");
}

sync_pid *sync_search(sync_pid **first, int val) {
    sync_pid *found = NULL;
    sync_pid *tmp;

    if (*first == NULL) {
        printk(KERN_INFO "Attention, sync pid queue is empty. sync_search()");
    } else {
        tmp = *first;

        while (tmp != NULL && !found) {
            if (val == tmp->pid) {
                found = tmp;
            }
            tmp = tmp->next;
        }
    }

    return found;
}

Message PriorityDequeue_sync(int target_arrival) {
    int i;   
    int stop = 0;  
    Queue_mes *tmp;
    Queue_mes *prev;
    Message target_sync_mes;
    Queue_mes *next_tmp;

    //Iterate through each priority queue
    for (i = 9; i >= 0 && stop == 0; i--) {
        tmp = PQ[i];
        prev = NULL;

        //Traverse the PQ[i]
        while(tmp != NULL && stop == 0){
            
            //Case: Found a message with matching recipient PID and delay is over         
            if (tmp->arrival == target_arrival){
                
                target_sync_mes = tmp->mes;

                //Case: Found message is the first in the PQ[i]
                if (prev == NULL) {
                    PQ[i] = tmp->next;
                } else {
                    prev->next = tmp->next;
                }

                //If the last pointer points to the target node, update it
                if (pq_last[i] == tmp) {
                    pq_last[i] = prev;
                }

                //Save the next pointer before freeing the node
                next_tmp = tmp->next;

                //Free the memory of the targeted node
                kfree(tmp);

                stop = 1;

            } else {
                prev = tmp;
                tmp = tmp->next;
            }
        }
    }

    return target_sync_mes;
}



//PQ functions: ##################################

Queue_mes **pqinit(void) {
    int i;

    //allocate memory for the array Queue_mes (**PQ)
    Queue_mes **myPQ = (Queue_mes **)kmalloc(10 * sizeof(Queue_mes *), GFP_KERNEL);

    if (myPQ == NULL) {
        printk(KERN_ERR "Error: memory in pqinit() not allocated.\n");
        return ERR_PTR(-EFAULT);
    }

    //allocate each PQ[i]
    for (i=0; i<10; i++) {
        myPQ[i] = (Queue_mes *)kmalloc(sizeof(Queue_mes), GFP_KERNEL);

        if (myPQ[i] == NULL) {
            printk(KERN_ERR "Error: memory in pqinit() not allocated.\n");
            kfree(myPQ);
            return ERR_PTR(-EFAULT);
        }

        myPQ[i] = NULL;
    }

    return myPQ;
}

void PriorityDequeue_mes(Queue_mes **first, int pid_caller) {
    int i;
    Queue_mes *tmp;
    Queue_mes *prev;
    Queue_buffer *new_node;
    Queue_mes *next_tmp;
    
    //Delay utilities
    unsigned int current_arrival;
    ktime_t current_time = ktime_get();
    s64 ns_since_boot = ktime_to_ns(current_time);
    current_arrival = ns_since_boot / 1000000000ULL;

    // Iterate through each priority queue
    for (i = 9; i >= 0; i--) {
        tmp = first[i];
        prev = NULL;

        // Traverse the PQ[i]
        while (tmp != NULL) {
            // Case: Found a message with matching recipient PID && delay consumed.
            if ( (tmp->mes.pid_recipient == pid_caller) && (current_arrival - tmp->arrival) > tmp->mes.delay) {
               
                //Allocate a new buffer queue node and enqueue the message pointer
                new_node = bq_init((tmp->mes));
                
                if (new_node != NULL) {
                    bq_enqueue(new_node);
                }

                //Case: Found message is the first in the PQ[i]
                if (prev == NULL) {
                    PQ[i] = tmp->next;
                } else {
                    prev->next = tmp->next;
                }

                //If the last pointer points to the target node, update it
                if (pq_last[i] == tmp) {
                    pq_last[i] = prev;
                }

                //Save the next pointer before freeing the node
                next_tmp = tmp->next;

                // Free the memory of the targeted node
                kfree(tmp);

                //Move to the next node in the list
                tmp = next_tmp;
            } else {
                prev = tmp;
                tmp = tmp->next;
            }

        }
    }
}

int sync_mes_find_check(int var){
    int i;
    int stop = 0;
    int found = 0;
    Queue_mes *tmp;
    Queue_mes *prev;

    //Iterate the lookup through each priority queue
    for(i = 9; i >= 0; i--){
        tmp = PQ[i];
        prev = NULL;

        // Traverse the PQ[i]
        while(tmp != NULL && stop == 0){
            
            //Case: Found a message with matching target PID            
            if (tmp->mes.pid_recipient == var){
                found = 1;
                stop = 1;
            }

            tmp = tmp->next;  
        }
    }

    return found;
}

changeData sync_mes_find(int target_pid) {
    int i;
    changeData box;
    Queue_mes *tmp;

    //Delay utilities
    unsigned int current_arrival;
    ktime_t current_time;
    s64 ns_since_boot;
    int time_since_enqueue;
    int time_before_left;
    int min_left = -1;
    int stop=0;

    //Iterate the lookup through each priority queue
    for(i = 9; i >= 0; i--){
        tmp = PQ[i];

        //Traverse the PQ[i]
        while (tmp != NULL && stop==0) {
            
            //Case: Found a message with matching target PID            
            if (tmp->mes.pid_recipient == target_pid) {

                //get the time "now"
                current_time = ktime_get();
                ns_since_boot = ktime_to_ns(current_time);
                current_arrival = ns_since_boot / 1000000000ULL;

                
                time_since_enqueue = (current_arrival - tmp->arrival);     //time PQ[i][X] is here
                time_before_left = (tmp->mes.delay - time_since_enqueue);  //remaining wait time
                
                //means that the message delay is expired
                if(time_before_left <= 0){
                    time_before_left=0;
                    min_left = time_before_left;
                    box.arrival= tmp->arrival;
                    box.delay = tmp->mes.delay;
                    box.effective_delay = min_left;
                    stop=1;
                }

                //first iteration to set for the first
                if(min_left == -1){
                    min_left = time_before_left;
                    box.arrival= tmp->arrival;
                    box.delay = tmp->mes.delay;
                    box.effective_delay = min_left;
                }

                //each iteration to find the min
                if(time_before_left < min_left){
                    min_left = time_before_left;
                    box.arrival = tmp->arrival;
                    box.delay = tmp->mes.delay;
                    box.effective_delay = time_before_left;
                }

            }
            
            tmp = tmp->next;
        }
    }

    return box;
}

void pq_print(Queue_mes **t) {
    int i;
    Queue_mes **temp = (Queue_mes **)kmalloc(10 * sizeof(Queue_mes *), GFP_KERNEL);

    if(temp == NULL){
        printk(KERN_ERR "Error: memory of pq_print() not allocated.\n");
        return;
    }

    //Copy the elements of t to temp
    for (i = 0; i < 10; i++) {
        temp[i] = t[i];
    }

    printk(KERN_INFO "\nMultiple-Queue_mess:\n");

    for (i = 0; i < 10; i++) {
        printk(KERN_INFO "PQ[%d]:  ", i);

        if (temp[i] == NULL) {
            printk(KERN_INFO "NULL\n");
        } else {
            while (temp[i] != NULL) {
                printk(KERN_INFO "%d  ", temp[i]->mes.priority);
                temp[i] = temp[i]->next;
            }
            printk(KERN_INFO "\n");
        }
    }

    kfree(temp);
}

void PriorityEnqueue_mes(Queue_mes **t, Message *m){
    int array_priority = m->priority - 1;
    Queue_mes *tmp;

    //Delay utilities
    unsigned int current_arrival;
    ktime_t current_time = ktime_get();
    s64 ns_since_boot = ktime_to_ns(current_time);
    current_arrival = ns_since_boot / 1000000000ULL;

    tmp = (Queue_mes *)kmalloc(sizeof(Queue_mes), GFP_KERNEL);

    if(tmp == NULL){
        printk(KERN_ERR "Error: memory in PriorityEnqueue_mes() not allocated.\n");
        return;
    }

    tmp->mes = (*m);
    tmp->arrival = current_arrival;
    tmp->next = NULL;

    // Empty Queue_mes[i]:
    if (t[array_priority] == NULL) {
        t[array_priority] = tmp;
        pq_last[array_priority] = tmp;
    } else {
        Queue_mes *prev = pq_last[array_priority];
        prev->next = tmp;
        pq_last[array_priority] = tmp;
    }
}

void pq_deinit(Queue_mes **myPQ) {
    Queue_mes *curr = NULL;
    Queue_mes *tmp = NULL;
    int i;

    // Free memory for each element in each Queue_mes
    for (i = 0; i < 10; i++) {
        curr = myPQ[i];

        while (curr != NULL) {
            tmp = curr;
            curr = curr->next;
            kfree(tmp);
        }

        myPQ[i] = NULL;
    }

    // Free memory for array of Queue_mes pointers
    kfree(myPQ);

    printk(KERN_INFO "\nMemory released correctly. pq_deinit()\n");
}



//Buffer Queue functions: ##################################

Queue_buffer *bq_init(Message tmp) {
    Queue_buffer *new_node = (Queue_buffer *)kmalloc(sizeof(Queue_buffer), GFP_KERNEL);
    
    if (new_node == NULL) {
        printk(KERN_ERR "Error: memory in bq_init() not allocated for buffer queue node.\n");
        return ERR_PTR(-EFAULT);
    }
    
    //Copy the Message to the new node's Message field
    new_node->mymes = tmp;
    new_node->next = NULL;
    
    return new_node;
}

void bq_enqueue(Queue_buffer *tmp) {
    Queue_buffer *curr;

    //empty buffer queue
    if (head == NULL) {
        head = tmp;

    //more items
    } else {
        curr = head;

        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = tmp;
    }

}

Message *bq_dequeue(void) {
    Queue_buffer *curr;
    Message *msg;
    
    //Buffer queue is empty
    if (head == NULL) {
        printk(KERN_INFO "Attention, no elements in buffer queue.\n");
        return NULL; 
    }
    
    curr = head;
    head = curr->next;

    msg = &(curr->mymes);

    return msg;
}



module_init(ipc_os_module_init);
module_exit(ipc_os_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Cattoni, Dennis Cattoni, Manuel Vettori");
MODULE_DESCRIPTION("IPC-SO");
