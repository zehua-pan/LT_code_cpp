# Introduction 
## about fountain codes
This project aims to use c++ programming language to implement Luby Transform code under UDP transfer protocol.
Luby Transform code is one of the basic fountain codes which are known as rateless erasure codes; 
Together with UDP protocal, Luby Transform code can be applied to many practical scenarios like network 
where there is physically only one way of communication or a spatical transmission.
## about UDP implementation in this project
LT(Luby Transform) code is usually applied to network transmission. 
Since UDP protocal in this project is needed, we must implement code for two sides : client and server.
Client encodes the file we want to send and server decodes the data from client and recover the original file for us.
By this way, we can configure client and server in any locations we want by setting them appropriately. 
For convenience, this project only sets local network(127.0.0.1) as middle network;
But client and server have two completely independent sets of codes which must be built and executed seperately.
Therefore, people who want to apply this project to a real network can change the socket setting in the code and implement their ideas easily.


# Getting Started
##	Installation process
install all codes to your local computer;
##	build and test

# developed platform
+ operating system: Ubuntu20.04 (Linux)
+ programming language: c++14

# File and directory explanation
## file structure of this project
```
project
+-- README.md
+-- noUDP
|   +-- *.cpp
|   +-- *.h
+-- withUDP
|   +-- client 
|   |   +-- Encoder.h
|   |   +-- Sender.h
|   |   +-- DegreeGenerator.h
|   |   +-- globalParameters.h
|   +-- server
|   |   +-- Decoder.h
|   |   +-- Receiver.h
|   |   +-- globalParameters.h
```
## code structure explanation
This project has two main directories. 
One is "noUDP". This directory doesn't make use of UDP. 
It is a quick implementation of LT code.
Codes in this directory only generates enough symbols in local memory and decode them according to the algorithm.

The other one is "withUDP". This drectory contains the two complete sets of code for client and server
and make use of UDP protocol to transfer the data.






