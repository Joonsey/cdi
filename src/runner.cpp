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
			id = 0;
			quitting = false;
		}

	Packet::Packet receive_response() {
		std::string error;
		char buffer[1024];
		int bytes_received = recv(client_socket, buffer, 1024, 0);

		if (bytes_received <= 0) {
			Packet::Packet packet(id, "UNEXPECTED", Packet::TYPE::ERROR);
			packet.serialize(error);
			exit(1);
		}

		return Packet::Packet::deserialize(buffer);
	}

	void start_loop_threaded() {
		std::thread main_thread(&TCPClient::main_loop, this);
		p_main_thread = &main_thread;
		main_thread.detach();
	}

	void start_loop() {
		main_loop();
	}

private:
	void init_connection() {
		std::string init_message;

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

		Packet::Packet packet(INIT_ID, "hello, world", Packet::TYPE::INIT);
		packet.serialize(init_message);

		send_data(init_message);
	}

	void main_loop()
	{
		while (!quitting) {
			Packet::Packet response_packet = receive_response();
			std::string response;

			if (response_packet.get_type() == Packet::TYPE::INIT_SUCCESS) {
				id = response_packet.get_id();
			}

			else if (response_packet.get_type() == Packet::TYPE::INIT_FAILURE) {
				close(client_socket);
				quitting = true;
			}

			if (response_packet.get_type() == Packet::TYPE::FETCH) {
				int status = fetch();
				std::string data = PREFIX;
				data.append(std::to_string(status));
				Packet::Packet pack(id, data, Packet::TYPE::FETCH);
				pack.serialize(response);
				send_data(response);
			}

			std::cout << "ID: "   << response_packet.get_id()   << std::endl;
			std::cout << "TYPE: " << response_packet.get_type() << std::endl;
			std::cout << "DATA: " << response_packet.get_data() << std::endl;
		}
	}

	void send_data(std::string& data) {
		ssize_t status = send(client_socket, data.data(), data.size(), 0);

		if (status == -1) {
			std::cout << status << std::endl;
            std::cerr << "Error sending data to server" << std::endl;
		}
	}

	int fetch()
	{
		return 200;
	}

    const char* server_ip;
	std::thread* p_main_thread;
    int server_port;
    int client_socket;
	int id;
	bool quitting;
};

int main() {
    TCPClient client("127.0.0.1", PORT);
	client.start_loop();
    return 0;
}

