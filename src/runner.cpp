#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <queue>

#include "packet.hpp"
#include "utils.hpp"

#define PORT 6969

struct StatusInfo {
	Orchestrator::STATUS status;
	std::string head;
};

class QueueTask {
	public:
		virtual void process(StatusInfo*) = 0;
};

class FetchTask : public QueueTask {
	public:
		void process(StatusInfo* status_ptr) override {
			(*status_ptr).status = Orchestrator::STATUS::FETCHING;
			sleep(5);
		};
};

class PullTask : public QueueTask {
	public:
		void process(StatusInfo* status_ptr) override {
			(*status_ptr).status = Orchestrator::STATUS::PULLING;
			sleep(5);
		};
};

class DeployTask : public QueueTask {
	public:
		void process(StatusInfo* status_ptr) override {
			(*status_ptr).status = Orchestrator::STATUS::DEPLOYING;
		};
};

class TCPClient {
public:
    TCPClient(const char* server_ip, int server_port)
		: server_ip(server_ip)
		, server_port(server_port)
		, status({Orchestrator::STATUS::PENDING, "db9685c"})
		, id(0)
		, quitting(false)
		{
			init_connection();
		}

	Packet::Packet receive_response() {
		std::string error;
		char buffer[1024] = {0};
		int bytes_received = recv(client_socket, buffer, 1024, 0);

		if (bytes_received <= 0) {
			Packet::Packet packet(id, "UNEXPECTED", Packet::TYPE::ERROR);
			packet.serialize(error);
			exit(1);
		}

		return Packet::Packet::deserialize(buffer);
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

		Packet::Packet packet(INIT_ID, "hello/world", Packet::TYPE::INIT);
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
				working_queue.push(new FetchTask());
			}

			if (response_packet.get_type() == Packet::TYPE::PULL) {
				working_queue.push(new PullTask());
			}

			if (response_packet.get_type() == Packet::TYPE::DEPLOY) {
				working_queue.push(new DeployTask());
			}

			/*
			if (response_packet.get_type() == Packet::TYPE::STATUS) {
				std::string data = produce_status();
				Packet::Packet pack(id, data, Packet::TYPE::STATUS);
				pack.serialize(response);
				send_data(response);
			}
			*/
			if (!working_queue.empty() && status.status == Orchestrator::STATUS::PENDING)
			{
				wake_up_working_queue();
			}

			Packet::Packet pack(id, produce_status() , Packet::TYPE::STATUS);
			pack.serialize(response);
			send_data(response);

			std::cout << "ID: "   << response_packet.get_id()   << std::endl;
			std::cout << "TYPE: " << response_packet.get_type() << std::endl;
			std::cout << "DATA: " << response_packet.get_data() << std::endl;
		}
	}

	void start_working_queue() {
		while (!quitting) {
			if (working_queue.empty())
			{
				status.status = Orchestrator::STATUS::PENDING;
				return;
			}

			auto current_task = working_queue.front();
			current_task->process(&status);
			working_queue.pop();
			free(current_task);
		}
	}

	void wake_up_working_queue() {
		std::thread main_thread(&TCPClient::start_working_queue, this);
		p_main_thread = &main_thread;
		main_thread.detach();
	}

	void send_data(std::string& data) {
		ssize_t status = send(client_socket, data.data(), data.size(), 0);

		if (status == -1) {
			std::cout << status << std::endl;
            std::cerr << "Error sending data to server" << std::endl;
		}
	}

	int fetch() {
		return 200;
	}

	std::string produce_status() {
		// S<latest_commit>|<status>
		// needs git integration to complete
		return format_string("S%s|%s", {status.head, std::to_string(status.status)});
	}


    const char* server_ip;
	std::thread* p_main_thread;
    int server_port;
    int client_socket;
	int id;
	bool quitting;
	std::queue<QueueTask*> working_queue;
	StatusInfo status;
};

int main() {
    TCPClient client("127.0.0.1", PORT);
	client.start_loop();
    return 0;
}

