#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <sstream>  // Include this header for std::istringstream
#include <set>
#include <algorithm>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

struct ClientInfo {
    std::string type;
    std::string topic;
};

void start_server(int port) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << port << std::endl;

    fd_set readfds;
    std::vector<int> clients;
    std::map<int, ClientInfo> client_info;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int client_fd : clients) {
            FD_SET(client_fd, &readfds);
            if (client_fd > max_sd) {
                max_sd = client_fd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            std::cout << "New connection, socket fd is " << new_socket << std::endl;
            clients.push_back(new_socket);

            // Read client type and topic
            char type_buffer[BUFFER_SIZE] = {0};
            int valread = read(new_socket, type_buffer, BUFFER_SIZE);
            if (valread > 0) {
                ClientInfo info;
                std::istringstream iss(type_buffer);
                std::getline(iss, info.type, ' ');
                std::getline(iss, info.topic);
                client_info[new_socket] = info;
            }
        }

        for (auto it = clients.begin(); it != clients.end();) {
            int client_fd = *it;
            if (FD_ISSET(client_fd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int valread = read(client_fd, buffer, BUFFER_SIZE);
                if (valread <= 0) {
                    close(client_fd);
                    it = clients.erase(it);
                    client_info.erase(client_fd);
                    std::cout << "Client disconnected" << std::endl;
                } else {
                    std::string message(buffer);
                    ClientInfo info = client_info[client_fd];
                    std::cout << "Received from client " << client_fd << " (" << info.type << ", " << info.topic << "): " << message << std::endl;

                    if (info.type == "PUBLISHER") {
                        for (int subscriber_fd : clients) {
                            ClientInfo sub_info = client_info[subscriber_fd];
                            if (sub_info.type == "SUBSCRIBER" && sub_info.topic == info.topic) {
                                send(subscriber_fd, message.c_str(), message.length(), 0);
                            }
                        }
                    }
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <PORT>" << std::endl;
        return EXIT_FAILURE;
    }
    int port = std::stoi(argv[1]);
    start_server(port);
    return 0;
}
