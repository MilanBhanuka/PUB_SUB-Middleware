#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void start_client(const char* server_ip, int server_port, const std::string& client_type, const std::string& topic) {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server " << server_ip << ":" << server_port << std::endl;

    // Send client type and topic to server
    std::string client_info = client_type + " " + topic;
    send(client_fd, client_info.c_str(), client_info.length(), 0);

    if (client_type == "PUBLISHER") {
        while (true) {
            std::cout << "Enter message to send to server (type 'terminate' to end): ";
            std::string message;
            std::getline(std::cin, message);
            send(client_fd, message.c_str(), message.length(), 0);
            if (message == "terminate") {
                std::cout << "Terminating connection" << std::endl;
                close(client_fd);
                break;
            }
        }
    } else if (client_type == "SUBSCRIBER") {
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(client_fd, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                std::cout << "Connection closed by server" << std::endl;
                close(client_fd);
                break;
            }
            std::cout << "Received from server: " << buffer << std::endl;
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <SERVER_IP> <SERVER_PORT> <PUBLISHER/SUBSCRIBER> <TOPIC>" << std::endl;
        return EXIT_FAILURE;
    }
    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    std::string client_type = argv[3];
    std::string topic = argv[4];
    start_client(server_ip, server_port, client_type, topic);
    return 0;
}
