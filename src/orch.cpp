#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "packet.hpp"

#define PORT 6969

class TCPServer {
	public:
		TCPServer(int port) : port(port) {}

		void start() {
			server_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (server_socket == -1) {
				std::cerr << "Error creating socket" << std::endl;
				return;
			}

			sockaddr_in server_address;
			server_address.sin_family = AF_INET;
			server_address.sin_port = htons(port);
			server_address.sin_addr.s_addr = INADDR_ANY;

			if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
				std::cerr << "Error binding socket" << std::endl;
				return;
			}

			if (listen(server_socket, 10) == -1) {
				std::cerr << "Error listening on socket" << std::endl;
				return;
			}

			std::cout << "Server listening on port " << port << "..." << std::endl;

			while (true) {
				sockaddr_in client_address;
				socklen_t clientAddrSize = sizeof(client_address);
				int clientSocket = accept(server_socket, (struct sockaddr*)&client_address, &clientAddrSize);
				if (clientSocket == -1) {
					std::cerr << "Error accepting connection" << std::endl;
					continue;
				}
				std::cout << "Connection accepted from " << inet_ntoa(client_address.sin_addr) << std::endl;

				std::thread client_thread(&TCPServer::handle_client, this, clientSocket);
				client_thread.detach();
			}
		}

private:
    void handle_client(int client_socket) {
        char buffer[1024];
		std::string response;
        while (true) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                close(client_socket);
                break;
            }

			Packet::Packet packet = Packet::Packet::deserialize(buffer);

			std::cout << "ID: "   << packet.get_id()   << std::endl;
			std::cout << "TYPE: " << packet.get_type() << std::endl;
			std::cout << "DATA: " << packet.get_data() << std::endl;

			if (packet.get_type() == Packet::TYPE::INIT) {
				Packet::Packet response_packet(packet.get_id(), packet.get_data(), Packet::TYPE::INIT_SUCCESS);
				packet.serialize(response);
			}

			if (packet.get_type() == Packet::TYPE::FETCH) {
				std::cout << stoi(packet.get_data()) << std::endl;
			}

			if (response.empty()) return;
			send_data(client_socket, response);
        }
    }

	void send_data(int socket, std::string& data) {
		ssize_t status = send(socket, data.data(), data.size(), 0);

		if (status == -1) {
			std::cout << status << std::endl;
            std::cerr << "Error sending data to client" << std::endl;
		}
	}

    int port;
    int server_socket;
};

int main() {
	TCPServer server(PORT);
	server.start();
	return 0;
}
