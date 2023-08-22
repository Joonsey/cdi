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

struct Connection {
	int socket;
	int id;
};

class TCPServer {
	public:
		TCPServer(int port) : port(port), oita(START_ID){
		}

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
				exit(1);
				return;
			}

			std::cout << "Server listening on port " << port << "..." << std::endl;

			while (true) {
				sockaddr_in client_address;
				socklen_t clientAddrSize = sizeof(client_address);
				int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &clientAddrSize);
				if (client_socket == -1) {
					std::cerr << "Error accepting connection" << std::endl;
					continue;
				}
				std::cout << "Connection accepted from " << inet_ntoa(client_address.sin_addr) << std::endl;

				std::thread client_thread(&TCPServer::handle_client, this, client_socket);
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

		void print_connections()
		{
			for (const auto& entry : connections) {
			const std::string& key = entry.first;
			const Connection& conn = entry.second;
			std::cout << "Key: " << key << ", Id: " << conn.id << ", Socket: " << conn.socket << std::endl;
			}
		}

	std::unordered_map<std::string, Connection> connections;

private:
    void handle_client(int client_socket) {
        char buffer[1024];
		std::string response;
        while (true) {
			memset(buffer,0,sizeof(buffer));
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
				remove_socket_from_connections(client_socket);
                close(client_socket);
				std::cout << "client disconnected" << std::endl;
                break;
            }

			Packet::Packet packet = Packet::Packet::deserialize(buffer);

			std::cout << "ID: "   << packet.get_id()   << std::endl;
			std::cout << "TYPE: " << packet.get_type() << std::endl;
			std::cout << "DATA: " << packet.get_data() << std::endl;

			if (packet.get_type() == Packet::TYPE::INIT) {
				if (connections.find(packet.get_data()) != connections.end()) {
					std::cout << "client with name already connected" << std::endl;
					Packet::Packet response_packet(packet.get_id(), "client with name already connected", Packet::TYPE::INIT_FAILURE);
					response_packet.serialize(response);
					send_data(client_socket, response);
					return;
				}

				int new_id = init_new_id();

				connections[packet.get_data()] = {client_socket, new_id};
				Packet::Packet response_packet(new_id, packet.get_data(), Packet::TYPE::INIT_SUCCESS);
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
		oita++;
		return oita;
	}

	void remove_socket_from_connections(int socket_to_rm)
	{
		std::string key_to_rm;
		for (const auto& entry : connections) {
			const std::string& key = entry.first;
			const Connection& conn = entry.second;

			if (socket_to_rm == conn.socket)
			{
				key_to_rm = key;
				break;
			}
		}

		connections.erase(key_to_rm);
	}

	int oita;
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

	CROW_ROUTE(app, "/_print_conns")([&](){
		server.print_connections();
		return "success";
			});

	CROW_ROUTE(app, "/fetch")([&](){
		server.query_fetch(temp_client_sock, 200);
		return "haha";
			});

	app.port(EXTERN_PORT).run();


	return 0;
}
