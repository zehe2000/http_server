#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

// Include headers.
#include "compression.hpp"
#include "fileUtils.hpp"

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr.
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
  
    std::cout << "Logs from your program will appear here!\n";

    // Create a server socket.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    // Set up server address.
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);
  
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }
  
    /*************** Main server loop ***************/
    while (true) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
  
        std::cout << "Waiting for a client to connect...\n";
  
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        std::cout << "Client connected\n";

        // Fork a new process to handle the client.
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed\n";
            close(client_fd);
            continue;
        } else if (pid > 0) {
            close(client_fd);
        } else {
            // Child process

            // Create buffer and receive HTTP request.
            char buffer[1024];
            ssize_t bytesReceived = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytesReceived < 0) {
                std::cerr << "Error in recv()\n";
                close(client_fd);
                close(server_fd);
                return 1;
            }
  
            std::cout << "print buffer: " << buffer << " end buffer" << std::endl;
            std::string clientMessage(buffer);
            std::string httpResponse;

            if (clientMessage.starts_with("GET / HTTP/1.1")) {
                httpResponse = "HTTP/1.1 200 OK\r\n\r\n";

            // POST /files endpoint to write a file.
            } else if (clientMessage.rfind("POST /files/", 0) == 0) {
                std::string baseDirectory = argv[2];
                const std::string prefix = "POST /files/";
                std::size_t start = prefix.size();
                std::size_t endOfFilename = clientMessage.find(' ', start);
                if (endOfFilename == std::string::npos) {
                    httpResponse = "HTTP/1.1 400 Bad Request\r\n\r\n";
                } else {
                    std::string fileName = clientMessage.substr(start, endOfFilename - start);
                    fs::path filePath = fs::path(baseDirectory) / fileName;

                    const std::string clHeader = "Content-Length: ";
                    std::size_t clPos = clientMessage.find(clHeader);
                    std::size_t contentLength = 0;
                    if (clPos != std::string::npos) {
                        clPos += clHeader.size();
                        std::size_t clEnd = clientMessage.find("\r\n", clPos);
                        if (clEnd != std::string::npos) {
                            std::string clValueStr = clientMessage.substr(clPos, clEnd - clPos);
                            try {
                                contentLength = std::stoul(clValueStr);
                            } catch (...) {
                                httpResponse = "HTTP/1.1 400 Bad Request\r\n\r\n";
                            }
                        }
                    }

                    std::size_t headersEnd = clientMessage.find("\r\n\r\n");
                    if (headersEnd == std::string::npos) {
                        httpResponse = "HTTP/1.1 400 Bad Request\r\n\r\n";
                    } else {
                        std::string requestBody = clientMessage.substr(headersEnd + 4, contentLength);
                        std::ofstream outFile(filePath);
                        if (!outFile.is_open()) {
                            std::cerr << "Error: Could not create file at " << filePath << std::endl;
                            return 1;
                        }
                        outFile << requestBody;
                        outFile.close();
                        httpResponse = "HTTP/1.1 201 Created\r\n\r\n";
                    }
                }

            // GET /files endpoint to return a file.
            } else if (clientMessage.starts_with("GET /files")) {
                int begin = 11;
                std::string::size_type pos = clientMessage.find("HTTP/1.1");
                int end = (pos != std::string::npos) ? static_cast<int>(pos) : -1;
                std::string fileName = clientMessage.substr(begin, end - begin - 1);
                std::cout << "The file name: " << fileName << std::endl;
                std::string baseDirectory = argv[2];
                fs::path filePath = fs::path(baseDirectory) / fileName;
    
                if (!fs::exists(filePath)) {
                    httpResponse = "HTTP/1.1 404 Not Found\r\n\r\n";
                } else {
                    try {
                        std::string fileContent = readFileToString(filePath.string());
                        std::ostringstream oss;
                        oss << "HTTP/1.1 200 OK\r\n"
                            << "Content-Type: application/octet-stream\r\n"
                            << "Content-Length: " << fileContent.length() << "\r\n\r\n"
                            << fileContent;
                        httpResponse = oss.str();
                    } catch (const std::exception &e) {
                        std::cerr << "Error reading file: " << e.what() << std::endl;
                        return 1;
                    }
                }
    
            // GET /echo endpoint with optional gzip compression.
            } else if (clientMessage.starts_with("GET /echo")) {
                int begin = 10;
                std::string::size_type pos = clientMessage.find("HTTP/1.1");
                int end = (pos != std::string::npos) ? static_cast<int>(pos) : -1;
                std::string echoStr = clientMessage.substr(begin, end - begin - 1);
                bool supportsGzip = clientMessage.find("gzip") != std::string::npos;
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n"
                    << "Content-Type: text/plain\r\n";
                if (supportsGzip) {
                    std::string compressed = gzipCompress(echoStr);
                    oss << "Content-Encoding: gzip\r\n"
                        << "Content-Length: " << compressed.size() << "\r\n\r\n"
                        << compressed;
                } else {
                    oss << "Content-Length: " << echoStr.size() << "\r\n\r\n"
                        << echoStr;
                }
                httpResponse = oss.str();
    
            // GET /user-agent endpoint.
            } else if (clientMessage.starts_with("GET /user-agent")) {
                std::string::size_type pos_begin = clientMessage.find("User-Agent: ");
                int begin = (pos_begin != std::string::npos) ? (static_cast<int>(pos_begin) + 12) : -1;
                std::string clientMessageSub = clientMessage.substr(begin, clientMessage.length()-1);
                std::string::size_type pos_end = clientMessageSub.find("\r\n\r\n");
                int end = (pos_end != std::string::npos) ? static_cast<int>(pos_end) : -1;
                std::string userAgent = clientMessage.substr(begin, end);
                std::cout << userAgent << " with len:" << userAgent.length() << std::endl;
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n"
                    << "Content-Type: text/plain\r\n"
                    << "Content-Length: " << userAgent.length() << "\r\n\r\n"
                    << userAgent;
                httpResponse = oss.str();
            } else {
                httpResponse = "HTTP/1.1 404 Not Found\r\n\r\n";
            }

            // Send HTTP response.
            ssize_t bytesSent = send(client_fd, httpResponse.c_str(), httpResponse.size(), 0);
            if (bytesSent < 0) {
                std::cerr << "Error in send()\n";
            }
  
            close(server_fd);
            close(client_fd);
            return 0;
        }
    }
}
