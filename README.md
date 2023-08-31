# Server Socket Property

## Table of Contents
- [Introduction](#introduction)
- [Directory Structure](#directory-structure)
- [Program Details](#program-details)
  - [Servant](#servant)
  - [Server](#server)
  - [Client](#client)

## Introduction
The project implements a distributed system for processing real-estate transaction data. It involves three main components: servant processes that handle specific subsets of data, a server that coordinates requests, and a client that sends queries to the server. The system uses sockets for communication and is designed to efficiently process and respond to queries about real-estate transactions.

## Directory Structure
The dataset is structured as a directory containing city sub-directories, each with ASCII files representing real-estate transactions on specific dates. Each transaction record includes information about the transaction ID, real-estate type, street name, surface area, and price.

## Program Details

### Servant
The servant process is responsible for loading and managing a subset of the dataset. It handles specific cities and connects to the server using the provided IP and port. The servant communicates its responsibilities to the server, including the range of cities it handles and the port number the server should use to contact it. It then listens for incoming connections from the server and handles requests. The servant process will terminate when it receives a SIGINT signal.

### Server
The server process listens for incoming connections from both servant processes and client processes. It creates a pool of threads to handle incoming connections and distribute the workload. The server can identify whether the incoming connection is from a servant or a client. It manages information about the dataset that each servant process is responsible for. When a client query arrives, the server determines which servant processes to contact for the required information, forwards the request, collects responses, and sends the cumulative answer back to the client. The server process terminates gracefully upon receiving a SIGINT signal.

### Client
The client process reads queries from a specified request file. It creates threads for each query and synchronizes them to send requests to the server simultaneously. Each thread establishes a connection with the server, sends the query, receives the response, and prints it to STDOUT. After all threads have completed, the client process terminates.


