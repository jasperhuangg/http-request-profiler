#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <ctime>

#include "get.h"

#define PORT 80
#define MAX_CONTENT_LENGTH 131072 // 128 KB

void error(const char *msg) {
    perror(msg);
    exit(0);
}

std::string parseHost(std::string url) {

    // ---- Remove http:// or https:// from the start iff exists
    bool startsWithHttp = url.substr(0, 7) == "http://";
    bool startsWithHttps = url.substr(0, 8) == "https://";

    std::string parsedStart = url;
    if (startsWithHttp)
        parsedStart = url.substr(7, url.length() - 7);
    else if (startsWithHttps)
        parsedStart = url.substr(8, url.length() - 8);

    // ---- Remove anything after hostname
    int i = parsedStart.length() - 1;
    while(i != -1) {
        if (parsedStart[i] == '.')
            return parsedStart;
        if (parsedStart[i] == '/' || parsedStart[i] == ':')
            break;
        i--;
    }
    return parsedStart.substr(0, i);
}

std::string parsePath(std::string url) {

    int i = url.length() - 1;
    while(i != -1) {
        if (url[i] == '.')
            return "/ ";
        if (url[i] == '/')
            break;
        i--;
    }
    return url.substr(i, url.length() - i) + " ";
}

std::string constructMessage(std::string host, std::string path) {

    return "GET " + path + "HTTP/1.1" + 
    "\r\nHost: "+ host + 
    "\r\nConnection: close" + 
    "\r\n\r\n";
}

std::pair<std::string, int> get(std::string url) {

    // ---- Create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // ---- Look up (and parse) the server host, path
    std::string host = parseHost(url);
    std::string path = parsePath(url);
    struct hostent *server = gethostbyname(host.c_str());
    if (server == NULL)
        error("ERROR, no such host");

    // ---- Fill in the server address structure
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // ---- Connect the socket to the given url's host
    const int didConnect = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (didConnect < 0) error("ERROR connecting to host");

    // ---- Start the timer
    std::chrono::milliseconds start = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );

    // ---- Send the GET request byte by byte
    int bytes, sent, received;
    char response[MAX_CONTENT_LENGTH];
    int total = sizeof(response) - 1;
    sent = 0;
    std::string message = constructMessage(host, path);
    do {
        bytes = write(sockfd, message.c_str() + sent, total - sent);
        if (bytes < 0) error("ERROR writing message to socket");
        if (bytes == 0) break;
        sent += bytes;
    } while (sent < total);

    // ---- Receive the response byte by byte
    memset(response, 0, sizeof(response));
    total = sizeof(response) - 1;
    received = 0;
    do {
        bytes = read(sockfd, response + received, total - received);
        if (bytes <= 0)
            break;
        received += bytes;

    } while (received < total);
    if (received == total)
        error("ERROR storing complete response from socket (not enough space allocated)");

    // ---- Stop the timer
    std::chrono::milliseconds end = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
    int requestTime = end.count() - start.count();

    // ---- Close the socket
    close(sockfd);

    std::string res(response);
    return std::pair <std::string, int>(res, requestTime);
}