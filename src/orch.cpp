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
#include "crow_all.h"

#define PORT 6969
#define EXTERN_PORT 8000

int temp_client_sock;

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
				exit(1);
				return;
			}

			if (listen(server_socket, 10) == -1) {
				std::cerr << "Error listening on socket" << std::endl;
				exit(1)
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

		void query_fetch(int socket, int runner_id) {
			std::string buffer;
			Packet::Packet packet(runner_id, "fetch", Packet::TYPE::FETCH);
			packet.serialize(buffer);

			send_data(socket, buffer);
		}

		void start_thread()
		{
			std::thread main_thread(&TCPServer::start, this);
			p_main_thread = &main_thread;
			main_thread.detach();
		}

private:
    void handle_client(int client_socket) {
        char buffer[1024];
		std::string response;
        while (true) {
			memset(buffer,0,sizeof(buffer));
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                close(client_socket);
				std::cout << "client disconnected" << std::endl;
                break;
            }

			Packet::Packet packet = Packet::Packet::deserialize(buffer);

			std::cout << "ID: "   << packet.get_id()   << std::endl;
			std::cout << "TYPE: " << packet.get_type() << std::endl;
			std::cout << "DATA: " << packet.get_data() << std::endl;

			if (packet.get_type() == Packet::TYPE::INIT) {
				// needs to take the data from the packet and make a reference in a hastable.
				// such that we can associate their own data with a concurrent connection (socket).
				Packet::Packet response_packet(init_new_id(), packet.get_data(), Packet::TYPE::INIT_SUCCESS);
				response_packet.serialize(response);
				send_data(client_socket, response);
			}

			if (packet.get_type() == Packet::TYPE::FETCH) {
				std::cout << stoi(packet.get_data().substr(1)) << std::endl;
			}

			if (response.empty()) return;

			temp_client_sock = client_socket;
        }
    }

	void send_data(int socket, std::string& data) {
		ssize_t status = send(socket, data.data(), data.size(), 0);

		if (status == -1) {
			std::cout << status << std::endl;
            std::cerr << "Error sending data to client" << std::endl;
		}
	}

	int init_new_id()
	{
		// calculate new id based on concurrent users
		return 2323;
	}

    int port;
    int server_socket;
	std::thread* p_main_thread;
};

int main() {

	TCPServer server(PORT);
	server.start_thread();

	crow::SimpleApp app;
	app.loglevel(crow::LogLevel::Warning);

	CROW_ROUTE(app, "/")([](){
		return "hello, world";
			});

	CROW_ROUTE(app, "/fetch")([&](){
		server.query_fetch(temp_client_sock, 200);
		return "haha";
			});

	app.port(EXTERN_PORT).run();


	return 0;
}
