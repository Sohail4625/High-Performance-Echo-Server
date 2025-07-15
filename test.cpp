#include <iostream>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define NUM_REQUESTS 100000
#define BUFFER_SIZE 1024

using namespace std;
using namespace chrono;

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    double total_latency = 0.0;
    
    for (int i = 0; i < NUM_REQUESTS; ++i) {
        strcpy(buffer, "Hello, Server!");
        auto start = high_resolution_clock::now();
        send(sockfd, buffer, strlen(buffer), 0);

        recv(sockfd, buffer, BUFFER_SIZE, 0);
        auto end = high_resolution_clock::now();

        duration<double> elapsed = end - start;
        total_latency += elapsed.count();
    }

    double avg_latency = total_latency / NUM_REQUESTS;
    cout << "Average Latency: " << avg_latency * 1000 << " ms" << endl;

    close(sockfd);
    return 0;
}
