#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <ctime>
#include <string>
#include <fstream>

using namespace std;

#define MAX_EVENTS 10
#define PORT 8080

ofstream log_file;
string cached_timestamp;
time_t last_time = 0;
void log(int type, const string& message) {
    time_t now = time(nullptr);
    if (now != last_time) {
        tm* now_tm = localtime(&now);
        cached_timestamp = to_string(now_tm->tm_year + 1900) + "-"
                         + to_string(now_tm->tm_mon + 1) + "-"
                         + to_string(now_tm->tm_mday) + " "
                         + to_string(now_tm->tm_hour) + ":"
                         + to_string(now_tm->tm_min) + ":"
                         + to_string(now_tm->tm_sec);
        last_time = now;
    }

    
    if (type == 1) {
        if (log_file.is_open()) {
            log_file << cached_timestamp << " " << message << "\n";
            log_file.flush();
        }
    } else {
        if (log_file.is_open()) {
            log_file << cached_timestamp << " [ERROR] " << message << "\n";
            log_file.flush();
        }
    }
}

int set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        log(0, "fcntl(F_GETFL) failed");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        log(0, "fcntl(F_SETFL) failed");
        return -1;
    }
    return 0;
}
int main() {
    log_file.open("logs.log", ios::app); // Open in append mode
    if (!log_file.is_open()) {
        cerr << "Failed to open logs.log for writing!" << endl;
        return -1; // Critical error if log file cannot be opened
    }
    cout<<"Hello";
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log(0, "Socket creation failed");
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log(0, "Bind failed");
        return -1;
    }

    if (set_non_blocking(server_fd) < 0) {
        log(0, "Failed to set server socket non-blocking");
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        log(0, "Listen failed");
        return -1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        log(0, "Failed to create epoll instance");
        return -1;
    }

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        log(0, "Failed to add server socket to epoll");
        return -1;
    }

    epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            log(0, "Epoll wait failed");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int current_fd = events[i].data.fd;

            if (current_fd == server_fd) {
                sockaddr_in client_addr{};
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr *)&client_addr, &addr_len);
                if (client_fd == -1) {
                    log(0, "Accept failed");
                    continue;
                }

                if (set_non_blocking(client_fd) < 0) {
                    log(0, "Failed to set client socket non-blocking");
                    close(client_fd);
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    log(0, "Failed to add client socket to epoll");
                    close(client_fd);
                    continue;
                }

                log(1, "Client connected");
            } else {
                char buffer[1024];
                ssize_t bytes_received = recv(current_fd, buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    if (bytes_received == 0) {
                        log(1, "Client disconnected");
                    } else {
                        log(0, "Error receiving data from client");
                    }

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
                    close(current_fd);
                } else {
                    ssize_t bytes_sent = send(current_fd, buffer, bytes_received, 0);
                    if (bytes_sent <= 0) {
                        log(0, "Error sending data to client");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
                        close(current_fd);
                    }
                    // log(1, "Sent and received " + to_string(bytes_sent) + " bytes");
                }
            }
        }
    }
    close(server_fd);
    close(epoll_fd);
    return 0;
}
