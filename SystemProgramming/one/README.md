# Practice One

This programming assignment is intended to give you experience with developing system programs that utilize some of the following UNIX features: signals, threads, timers, pipes, FIFOs, and I/O multiplexing for nonblocking I/O.

## Part 1

This part asks for developing a C/C++ program that exhibits some concurrency aspects. The required program can be invoked as

```sh
p1 nLine inputFile delay
```

When run, the program performs a number of iterations. In each iteration it reads from the `inputFile` the next `nLine` text lines and displays them on the screen. It then enters a delay period for the specified number of `delay` milliseconds. During the delay period, the program stays responsive to input from the keyboard, as detailed below. After the delay period ends, the program proceeds to the next iteration where it reads and displays the next group of `nLine` text lines, and then enters another delay period. Subsequent iterations follow the same pattern. More details are given below.

1. At the beginning (and end) of each delay period, the program displays a message to inform the user of its entering (respectively, leaving) a delay period.
2. During a delay period, the program loops on prompting the user to enter a command, and executes the entered command. So, depending on the length of the delay period, the user can enter multiple commands for the program to execute.
3. Each user command is either `quit`, or some other string that the program tries to execute as a shell command line. The quit command causes the program to terminate and exit to the shell. Other strings are processed by passing them to the shell using the standard I/O library function `popen`.
4. Other than forking a child process implied by calling `popen()`, the program should not fork a child process to achieve the desired behavior. The process, however, may have threads and/or use timers.

## Part 2

File sharing software systems allow a community of users to share files contributed by members. This part calls for implementing a small set of features that may be used in a client-server system that provides such a service. In the system, each user (a client program) uploads files of different objects (e.g., HTML files, images, sound clips, video clips, etc.) to be stored and distributed with the help of a server program. Each object has a unique name and an owner (the client program that succeeds in storing the file at the server).

You are asked to write a C/C++ program that implements the functionality of a client-server system. The program can be invoked to run as a server or run as a client:

```sh
p2 -s
p2 -c idNumber inputFile
```

The `idNumber` is a client identification number, and `inputFile` is a file that contains work that needs to be done by the clients. The server can serve at most `NCLIENT = 3` clients. Each client is assigned a unique `idNumber` in the range `[1, NCLIENT]`. The server is assumed to have `idNumber = 0`.

Data transfers between each client and the server use FIFOs. Clients do not communicate with each other. A FIFO named `fifo-0-x` carries data from the server to client $x$. Similarly, `fifo-x-0` carries data in the other direction (from client to server).

For simplicity, the needed $2 \times NCLIENT$ FIFOs may be created in the work directory using the shell command `mkfifo` prior to starting your program development.

### Input File Format

The input file is a common file shared among all intended clients. The file has the following format:

+ A line starting with `#` is a comment line (skipped).
+ Empty lines are skipped. For simplicity, an empty line has a single `\n` character.
+ Else, a line specifies a command line whose format and meaning matches one of the following cases:
  + `idNumber (put|get|delete) objectName`: only the client with the specified `idNumber` sends to the server the specified `get`, `put` or `delete` request of the named object. An object name has at most `MAXWORD = 32` characters.
  + `idNumber gtime`: only the client with the specified `idNumber` sends to the server a get *time request*.
  + `idNumber quit`: only the client with the specified `idNumber` should terminate normally.

### Packet Types

Communication in the system uses messages stored in formatted packets. Each packet has a type, and carries a (possibly empty) message. Your program should support the following packet types.

+ `GET`, `PUT` and `DELETE`: For a specified object name, a client executes a get, put, or delete command by sending a packet of the corresponding type, where the message specifies the object name. An error condition arises at the server when the client’s request asks for doing one of the following:
  + getting a non-existing object
  + putting an object that already exists
  + deleting an object owned by another client
+ `GTIME` and `TIME`: A client processes a get server's time command (`gtime`) by sending a``GTIME` packet (with an empty message). The server replies by sending a `TIME` packet where the message contains the time in seconds (a real number) since the server started operation.
+ `OK` and `ERROR`: The server replies with an OK packet if the received request is processed successfully. Else, the server replies with an ERROR packet with a suitable error description in the message part.
