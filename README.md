# tcp-compression-service
This implementation follows a simple producer-consumer pattern. Once the service is initialized, the server socket is registered with an epoll instance and a number of listener threads wait on the epoll until a new conneciton is requested.

Once a new connection is made it is registered with the epoll instance and will be handled once data is ready to be read. The header and payload are read from the client and passed to a worker thread to process the request. 

The worker thread will service the request based on the request type, and then provide a response to the client.

Note: The maximum request payload size is 4KiB

## Target Platform
This project was developed for Ubuntu 18.04 and built with the following:

- CMake version 3.10.2
- Make  version 4.1
- clang version 6
- C++ 17

To my knowledge there are no Ubuntu specific features and thus any flavor of Linux with a kernel version of 4.5 or greater should be supported. 

## External Dependencies
This project uses functionality from the Linux Kernel and C++ STL. It is required that the pthread library be availible on the system. 

## Assumptions
This project makes the following assumptions:
- When returning the stats of the service, the field Compression Ratio refers to the result of the most recent compression at the time the response is evaluated
- When resetting the stats of the service, the total bytes sent will be set to zero at the time the response is evaluated, thus after a response is sent to the client the total bytes sent will not be zero

## Room for Improvement
Given more time, here are some things that could be improved:
- DOS protection: Currently, should a malicious (or slow) actor create a number of connections equal to the pool of listeners and fail to send all the data they promised, the service will hang. This can be solved by storing the client's data in some shared buffer and collecting data through non-blocking recv calls, returning the client's fd to the epoll if there is nothing to read. This would also improve the uitilization of the listener threads as a side effect.
- Error Reporting: The use of the Unknown Error status code is not helpful to users of the service. With more time I would use the implementer-defined range of status codes to provide more specific error messages.
- Configuration Input: Currently, the best way to configure the service is through editing main.cpp and then rebuilding. With more time I would accept command line arguments or read from a config file.