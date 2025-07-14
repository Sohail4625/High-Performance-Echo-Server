#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

int main() {
    int port = 8080;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    int opt = 1;
    int sockp = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(sockp < 0) {
        perror("socket options");
        exit(EXIT_FAILURE);
    }
    int binding = bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr));
    if(binding < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    int list = listen(server_fd, 1);
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
    char buffer[1024];
    while (true) {
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);  
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                std::cout << "Client disconnected." << std::endl;
            } else {
                perror("recv");
            }
            break;
        }

        ssize_t bytes_sent = send(client_fd, buffer, bytes_received, 0);
        if (bytes_sent <= 0) {
            perror("send");
            break;  
        }
    }
    close(client_fd);
    close(server_fd);

    return 0;
}
