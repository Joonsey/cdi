#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "packet.hpp"

#define PORT 6969

class TCPClient {
public:
    TCPClient(const char* server_ip, int server_port)
		: server_ip(server_ip), server_port(server_port) {
			init_connection();
		}

    void connect_and_send() {
        for (int i = 0; i < 10; ++i) {
            std::string message = "Request " + std::to_string(i);
            send(client_socket, message.c_str(), message.size(), 0);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Delay between requests
        }

        close(client_socket);
    }

private:
	void init_connection() {
		std::string response;

        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(server_port);
        server_address.sin_addr.s_addr = inet_addr(server_ip);

        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
            std::cerr << "Error connecting to server" << std::endl;
            return;
        }

		Packet::Packet packet(228920, "hello, world", Packet::Type::INIT);
		packet.serialize(response);

		send_data(response);
	}

	void send_data(std::string& data) {
		ssize_t status = send(client_socket, data.data(), data.size(), 0);

		if (status == -1) {
			std::cout << status << std::endl;
            std::cerr << "Error sending data to server" << std::endl;
		}
	}

    const char* server_ip;
    int server_port;
    int client_socket;
};

int main() {
    TCPClient client("127.0.0.1", PORT);
    return 0;
}

