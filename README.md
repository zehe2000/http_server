# HTTP Server Project

This is a simple HTTP server written from scratch in C++ using low-level TCP primitives (sockets) on Linux. It demonstrates how to handle multiple concurrent connections, serve static files, and implement endpoints for basic request handling, including gzip compression.

## Table of Contents
1. [Overview](#overview)
2. [Project Structure](#project-structure)
3. [How It Works](#how-it-works)
4. [Endpoints](#endpoints)
5. [Build and Run Instructions](#build-and-run-instructions)
6. [License](#license)

---

## Overview

This project implements an HTTP server capable of:
- Handling **GET** and **POST** requests.
- Serving files under a specified directory.
- Handling multiple concurrent client connections using the Unix `fork()` mechanism.
- Providing optional **gzip** compression for echo responses.
- Logging basic server events (connections, request parsing, etc.).

It is meant as a demonstration or educational tool for understanding how HTTP and TCP/IP interplay, as well as how one might build and structure a simple server.

---

## Project Structure

Here’s a quick look at how the project is organized:

```text
http_server
├── .git/
└── src/
    ├── compression.cpp
    ├── compression.hpp
    ├── fileUtils.cpp
    ├── fileUtils.hpp
    └── server.cpp
```

---



## How It Works

The server operates following these steps:

1. **Initialization:**  
   - **Socket Creation:** A TCP socket is created using the `socket()` function.  
   - **Binding and Options:** The socket is bound to a specific IP address and port (default port 4221) and configured to allow address reuse with `setsockopt()`.

2. **Listening and Accepting:**  
   - **Listen:** The server listens for incoming connections using `listen()`.  
   - **Accept:** When a client connects, `accept()` returns a new client-specific socket.

3. **Connection Handling:**  
   - **Forking:** The server forks a new process using `fork()`, allowing the parent process to continue accepting new connections while the child process handles the current client.
   - **Request Parsing:** The child process reads the HTTP request from the client, determining the type of request (GET, POST, etc.) and the target endpoint.
  
4. **Response Generation:**  
   - **File Operations:** Depending on the endpoint, the server either reads from or writes to files in a specified base directory.
   - **Compression:** For the `/echo` endpoint, if gzip is supported by the client, the server compresses the response before sending.
   - **Status Codes:** Appropriate HTTP response codes (such as 200 OK, 201 Created, 400 Bad Request, or 404 Not Found) are returned based on the outcome of the operations.
  
5. **Cleanup:**  
   - The child process closes both its client and server sockets after processing the request, ensuring that resources are freed appropriately.

---

## Endpoints

The server supports several endpoints that demonstrate various HTTP operations:

- **GET /**  
  A basic endpoint that returns a simple 200 OK response, often used for health checks.

- **POST /files/&lt;filename&gt;**  
  Accepts a file's content in the request body and writes it to the specified `<filename>` in the base directory. Requires a `Content-Length` header to indicate the size of the payload.

- **GET /files/&lt;filename&gt;**  
  Retrieves the contents of `<filename>` from the base directory. If the file does not exist, the server returns a 404 Not Found response.

- **GET /echo/&lt;message&gt;**  
  Echoes back the `<message>` supplied in the URL. If the request includes `Accept-Encoding: gzip`, the response is compressed accordingly.

- **GET /user-agent**  
  Extracts and returns the User-Agent header from the client's HTTP request.

---

## Build and Run Instructions

### Prerequisites

- **Compiler:**  
  A C++ compiler with C++17 support or later.

- **Dependencies:**  
  The [zlib](https://zlib.net/) library is required for gzip compression.

- **Environment:**  
  Linux-based environment with support for socket APIs.

### Compilation

Compile the project from the terminal by navigating to the project directory (or the `src/` directory) and using the following command:

```bash
g++ -std=c++17 server.cpp fileUtils.cpp compression.cpp -lz -o http_server
```

### Testing the Server

Use tools like curl or any HTTP client to test the endpoints:

Health Check (GET /):
```
curl -v http://localhost:4221/
```

Uploading a File (POST /files/<filename>):

```
curl -X POST -H "Content-Length: 11" --data "hello world" http://localhost:4221/files/test.txt
```

Downloading a File (GET /files/<filename>):
```
curl http://localhost:4221/files/test.txt
```

Echo with Gzip Compression (GET /echo/<message>):
```
curl -H "Accept-Encoding: gzip" http://localhost:4221/echo/your_message_here
```

User-Agent Inspection (GET /user-agent):
```
curl http://localhost:4221/user-agent
```