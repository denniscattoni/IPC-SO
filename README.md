# Operating Systems

<br>
<br>

This repository contains all the sources created for the laboratory's project during the course "Sistemi Operativi", that took place during the accademic year 2022/2023 in University of Trento.

<br>

The purpose of the project was to create a new mechanism for inter-process communication without using the existing ones.
<br>

Open [**Requirements and Constraints**](https://github.com/denniscattoni/tmp_so/blob/main/utilities/requirements/LabOS2023_Progetto_desame.pdf) to access more details about the project's constraints and requirements.
<br><br>

Refer to the following report: [**aggiungi_pdf_link.pdf**](//mettilinkqui), for more details on the architecture, limitations, and structure of the project.
<br><br>

![Alt text](https://github.com/denniscattoni/tmp_so/blob/main/utilities/img/ipc-so-high-wallpaper.png)

<br>

To provide better documentation, we have chosen to use the [**Doxygen tool**](https://www.doxygen.nl/).

If you want to read the **Doxygen Documentation** for the code, check the generated PDF doc [here](https://github.com/SergioBrodesco/FdR-groupW/blob/master/docs/latex/refman.pdf).

<br>

To view the html page, you need to open the **index.html** file inside the **./IPC-SO/blob/master/doxygen/html/index.html** folder

<br>

```
cd ./IPC-SO/blob/master/doxygen/html/index.html
xdg-open index.html
```

<br>
<br>

# Environment setup
<br>

The project was developed on the following version of Ubuntu: 20.04.6 LTS (Focal Fossa). We do not guarantee proper functionality on versions different than this. If you do not have access to this version, you can download VirtualBox from the [Official Website](https://www.virtualbox.org/), then load and run the appropriate version of Ubuntu.

<br>

> [!NOTE]
> Before compiling or launching any files, ensure that you have set up the environment correctly to work with kernel modules.

<br>

Launch the following commands:

<br>

1. Update System Packages:

```
sudo apt update
sudo apt upgrade
```

<br>

2. Install Build Essential:

```
sudo apt install build-essential
```

<br>

3. Install Kernel Headers:

```
sudo apt install linux-headers-$(uname -r)
```

<br>

4. Reboot the system:

```
sudo reboot
```
<br>


If you followed the previous instructions correctly, everything should be properly set up and ready to work now.

<br>


# Compile and load

<br>

## Kernel Module:

<br>
It's time to download the repository, you should have a folder structure like this:

```
./IPC-SO
    |_ /utilities
    |_ /kernel
    |_ /user
```

Make sure to have the following files in the '/kernel' folder:

```
IPC_SO.h
ipc-so.c
Makefile
```

To compile the module, navigate to the path ./IPC-SO/kernel in the terminal and run the command:

```
make 
```

Once you have successfully compiled the module, you should see the file *IPC-SO.ko*. If you want to load the module into the system, run the command:

```
make insert
```

The others command allows the user to do the following utility operations:

* 'make delete': allows the user to unload the module from the system.
* 'make clean': allows the user to delete all the files in the ./kernel directory except these ones: *IPC_SO.h*, *ipc-so.c*, *Makefile*.

<br>
If you successfully loaded the module into the system, you should see the text 'IPC-SO module initialized' in the terminal.

<br><br>



## User Process:

<br>

Make sure to have the following files in the '/user' folder:

```
USER_PROCESS.h
user_process.c
Makefile
```

To compile the user process interface, navigate to the path ./IPC-SO/user in the terminal and run the command:

```
make 
```

The others command allows the user to do the following utility operations:

* 'make launch': allows the user to launch the program without root permission.
* 'make root': allows the user to launch the program with root permission.
* 'make clean': allows the user to delete all the files in the ./kernel directory except these ones: *USER_PROCESS.h*, *user_process.c*, *Makefile*.

<br>

Once you have successfully compiled the user_process.c, you should see the file *user_process.out*.

<br>


> [!NOTE]
> before launch the user_process.out make sure to have compiled the kernel module and loaded it correctly.

<br>

To be sure if the module is loaded correctly lauch the following command:

```
modinfo IPC_SO
```

<br>

You should see all the information about the IPC_SO module;

<br><br>





# Test

<br>
In order to test the IPC-SO module you have to have completed the previous instructions.
<br>

If you have loaded the module into the system correctly, you can run the *user_process.out* program an arbitrary number of times from a different teminal to create additional processes and enable communication among them.

<br>

This is an example of the user interface to interact with the module: 

<br>

![Alt text](https://github.com/denniscattoni/tmp_so/blob/main/utilities/img/ipcso_interface.png)

<br>

The interface is self-explanatory: 

<br>

When you run the *user_process.out* program, the initial action involves registration, where your process ID (pid) is recorded within the kernel module.

After the automatic registration the menu show you the operation you are able to do and you can try them following the operation numbers provided by the interface.

The main operations are: *_unregister_*, *_list of aviable processes_*, *_write_*, *_asynchronous read_*, *_synchronous read_*, *_exit_*.

<br>

After performing the *_unregister_* operation, the interface allows the user to either re-register with the system or exit.

<br>
<br>

![Alt text](https://github.com/denniscattoni/tmp_so/blob/main/utilities/img/ipcso_unreg_interface.png)

<br>
<br>

> [!NOTE]
> In the file ./IPC-SO/user/USER_PROCESS.h, you will find the *#define AUTO* set to 0. This prompts the user to manually input each field of the *Message struct*.
> 
> If you prefer automatic and random field generation, set the value of AUTO to 1. This will generate an automatic message where you only need to input the recipient's process ID.
> 
> This functionality is useful for testing and debugging purposes.

<br>
<br>

//TODO: fix the link to the line of code to the #define AUTO 1

[!Permalink](https://github.com/mfocchi/locosim/blob/c4853e7ce4ade118cfe46be4b3f99b4b003eeabb/robot_urdf/hyq.srdf#L9)

<br>
<br>

> [!NOTE]
> Priority level 10 is only allowed for processes with root permissions.
> If user_process.c is executed without sudo permissions (sudo ./user_process.) and the user attempts to assign a priority of 10 to a message, an "**Access denied**" error will occur.
>
> This situation can also arise in #define AUTO 1 mode, as the random function might assign priority level 10 to a message from a process that wasn't launched with sudo permissions.
>
> If you encounter the "**Access denied**" error during the testing phase, it's due to the specific reason outlined in the IPC-SO requirements.

<br>
<br>

> [!WARNING]  
> Please avoid abruptly terminating or improperly ending the execution of *user_process.out*.
>
> The current process's PID will still be saved in the kernel module, and this could lead to confusion for users, as the 'ghost' PID may continue to appear as available but it won't be.
>
> To terminate the execution correctly, use the appropriate operation in the interface.

<br>
<br>

If you encounter bugs or cases that are not managed well, please feel free to contact us! You can find our contact information in the [**credits section**](https://github.com/denniscattoni/tmp_so/blob/main/README.md#credits).

<br>
<br>



# References

<br>
To develop this project, we had to study new concepts and delve deeper into many Kernel programming notions.

<br>
<br>

The following guide was of fundamental importance for deepening some concepts necessary for the development of the project and to design the architecture:

<br>

* [**The Linux Kernel Module Programming Guide**](https://sysprog21.github.io/lkmpg/)

  credits to: Peter Jay Salzman, Michael Burian, Ori Pomerantz, Bob Mottram, Jim Huang

<br>

To gather more information about primitives and APIs, we referred to the official documentation available here:

<br>

* [**Kernel API**](https://www.kernel.org/doc/html/latest/core-api/index.html)
* [**Waiting Queues**](https://docs.huihoo.com/doxygen/linux/kernel/3.7/linux_2wait_8h.html)
  
<br>
This playlist was useful to show a trivial example of "Hello World" with a Kernel module:
<br><br>

* [**Kernel Module Development Playlist**](https://www.youtube.com/playlist?list=PL16941B715F5507C5)

<br>
<br>




# Credits

* Alex Cattoni - Università degli studi di Trento (Unitn), Trento – Italy
  <br> alex.cattoni@studenti.unitn.it

* Dennis Cattoni - Università degli studi di Trento (Unitn), Trento – Italy
  <br> dennis.cattoni@studenti.unitn.it

* Manuel Vettori - Università degli studi di Trento (Unitn), Trento – Italy
  <br> manuel.vettori@studenti.unitn.it



