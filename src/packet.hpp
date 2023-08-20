#include <iostream>
#include <vector>
#include <cstring>
#include <cstdint>
#define ID_SIZE 8
#define TYPE_SIZE 2
#define START_ID 1

namespace Packet
{
	enum STATUS {
		SUCCESS,
		FAILURE,
		PENDING
	};

	enum TYPE {
		INIT,
		INIT_SUCCESS,
		INIT_FAILURE,
		STATUS,
		FETCH,
		PULL,
		DEPLOY,
		WARNING,
		ERROR
	};

	class Packet  {
	public:
		Packet(unsigned int id, const std::string& data, TYPE type)
			: runner_id(id), packet_data(data), type(type) {}

		void serialize(std::string& buffer) const {
			std::string fake_buffer;
			std::string int_as_string;
			int_as_string = std::to_string(runner_id);
			fake_buffer.resize(ID_SIZE);
			std::fill( fake_buffer.begin(), fake_buffer.begin()+ID_SIZE, '0' );
			for (auto i = 0; i < int_as_string.size() + 1; i ++)
			{
				fake_buffer[fake_buffer.size() - i] = int_as_string[int_as_string.size() - i];
			}
			buffer.append(fake_buffer);

			int_as_string = std::to_string(type);
			fake_buffer.resize(TYPE_SIZE);
			std::fill( fake_buffer.begin(), fake_buffer.begin()+TYPE_SIZE, '0' );
			for (auto i = 0; i < int_as_string.size() + 1; i ++)
			{
				fake_buffer[fake_buffer.size() - i] = int_as_string[int_as_string.size() - i];
			}
			buffer.append(fake_buffer);

			buffer.append(packet_data);
		}

		static Packet deserialize(const std::string& buffer) {
			int runner_id;
			TYPE type;
			std::string packet_data, fake_type, fake_runner_id;

			fake_runner_id = buffer.substr(0, ID_SIZE);
			runner_id = std::stoi(fake_runner_id);

			fake_type = buffer.substr(ID_SIZE, ID_SIZE+TYPE_SIZE);
			type = static_cast<TYPE>(stoi(fake_type));

			packet_data = buffer.substr(ID_SIZE+TYPE_SIZE);

			return Packet(runner_id, packet_data, type);

		}

		int get_id() {
			return runner_id;
		}

		int get_type() {
			return type;
		}

		std::string get_data() {
			return packet_data;
		}

	private:
		int runner_id;
		TYPE type;
		std::string packet_data;
	};
}
