#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "lib/HTTPRequest.hpp"
#include <random>
#include <stdio.h>
#include <tuple>

#pragma comment(lib, "ws2_32.lib")
using json = nlohmann::json;
std::vector<unsigned char> HexToBytes(const std::string& hex) {
	std::vector<unsigned char> bytes;
	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		char byte = (char)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}
	return bytes;
}
std::string urlencode(const std::string& p)
{
	std::vector<unsigned char> s(HexToBytes(p));
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for (int i = 0, ix = s.size(); i < ix; i++)
	{
		const char& c = s[i];
		if ((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}
bool isASCII(const std::string& s)
{
	return !std::any_of(s.begin(), s.end(), [](char c) {
		return static_cast<unsigned char>(c) > 127;
		});
}
std::string string_to_hex(const std::string& in) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (size_t i = 0; in.length() > i; ++i) {
		ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(in[i]));
	}
	return ss.str();
}

json decode_bencoded_value(const std::string& encoded_value, int& chars_processed) {
    if (encoded_value.empty())
		return json::array();
	if (std::isdigit(encoded_value[0])) {
		// Example: "5:hello" -> "hello"
		size_t colon_index = encoded_value.find(':');
		if (colon_index != std::string::npos) {
			std::string number_string = encoded_value.substr(0, colon_index);
			int64_t number = std::atoll(number_string.c_str());
			std::string str = encoded_value.substr(colon_index + 1, number);
			chars_processed += 1 + number_string.length() + str.length();
			return json(str);
		}
		else {
			throw std::runtime_error("Invalid encoded value: " + encoded_value);
		}
	}
	else if (encoded_value[0] == 'i') {
		size_t end_index = encoded_value.find('e');
		std::string data = encoded_value.substr(1, end_index - 1);
		chars_processed += end_index;
		int64_t number = std::atoll(data.c_str());
		return json(number);
	} if (encoded_value[0] == 'l') {
		std::string content = encoded_value.substr(1, encoded_value.length());
		chars_processed += 1;
		auto parsed_content = json::array();
		while (content.length() != 0 && content[0] != 'e')
		{
			if (std::isdigit(content[0])) {
				size_t colon_index = content.find(':');
				int n = 0;
				auto decoded_value = decode_bencoded_value(content, n);
				parsed_content.emplace_back(decoded_value);
				auto decoded_string = decoded_value.template get<std::string>();
				chars_processed += n;
				content = content.substr(n, content.length());
			}
			else if (content[0] == 'i') {
				size_t end_index = content.find('e');
				std::string data = content.substr(1, end_index - 1);
				int64_t number = std::atoll(data.c_str());
                content = content.substr(2 + data.length(), content.length());
				chars_processed += 2 + data.length();
				parsed_content.emplace_back(number);
			}
			else if (content[0] == 'l') {
				int n = 0;
				auto decoded_list = decode_bencoded_value(content, n);
				parsed_content.emplace_back(decoded_list);
				content = content.substr(n, content.length());
				chars_processed += n;
				//std::cout << decoded_list.dump() << std::endl;
			}
			else if (content[0] == 'd') {
				int n = 0;
				auto decoded_list = decode_bencoded_value(content, n);
				parsed_content.emplace_back(decoded_list);
				content = content.substr(n, content.length());
				chars_processed += n;
				//std::cout << decoded_list.dump() << std::endl;
			}
        }
		content = content.substr(1, content.length());
		chars_processed += 1;
		return parsed_content;
	} if (encoded_value[0] == 'd') {
		std::string content = encoded_value.substr(1, encoded_value.length());
		chars_processed += 1;
		auto parsed_content = json::object();
        while (content.length() != 0 && content[0] != 'e') {
			std::string decoded_key = "";
            if (std::isdigit(content[0])) {
				size_t colon_index = content.find(':');
				int n = 0;
				auto decoded_value = decode_bencoded_value(content, n);
                auto decoded_string = decoded_value.template get<std::string>();
				decoded_key = decoded_value;
				chars_processed += n;
                content = content.substr(n, content.length());
			}
			else {
				throw std::runtime_error("Expected key as string: " + encoded_value);
			}
			auto decoded_dict_value = json::object();
			if (std::isdigit(content[0])) {
				size_t colon_index = content.find(':');
				int n = 0;
				auto decoded_value = decode_bencoded_value(content, n);
				decoded_dict_value = decoded_value;
				auto decoded_string = decoded_value.template get<std::string>();
				chars_processed += n;
				content = content.substr(n, content.length());
			}
			else if (content[0] == 'i') {
				size_t end_index = content.find('e');
				std::string data = content.substr(1, end_index - 1);
				int64_t number = std::atoll(data.c_str());
				content = content.substr(2 + data.length(), content.length());
				chars_processed += 2 + data.length();
                decoded_dict_value = json(number);
			}
			else if (content[0] == 'l') {
				int n = 0;
				auto decoded_list = decode_bencoded_value(content, n);
				content = content.substr(n, content.length());
				chars_processed += n;
                decoded_dict_value = decoded_list;
				//std::cout << decoded_list.dump() << std::endl;
			}
            else if (content[0] == 'd') {
				int n = 0;
				auto decoded_dict = decode_bencoded_value(content, n);
                decoded_dict_value = decoded_dict;
				content = content.substr(n, content.length());
				chars_processed += n;
            }
            parsed_content[decoded_key] = decoded_dict_value;
		}
        content = content.substr(1, content.length());
		chars_processed += 1;
        return parsed_content;
	}
	else {
		throw std::runtime_error("Unhandled encoded value: " + encoded_value);
	}
}
namespace ns {
    struct Info {
		std::int64_t length;
		std::string name;
		std::int64_t piece_length;
		std::string pieces;
    };
    void from_json(const json& j, Info& p) {
		j.at("name").get_to(p.name);
		j.at("length").get_to(p.length);
		j.at("piece length").get_to(p.piece_length);
        j.at("pieces").get_to(p.pieces);
	}
	std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(json object, bool compact = false) {
		if (compact) {
			std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
			std::string peers = object.template get<std::string>();
			for (int i = 0; i < peers.length(); i += 6) {
				std::string peer_ip = peers.substr(i, 4);
				std::string peer_port = peers.substr(i + 4, 2);
				std::uint8_t ip[4] = { 0 };
				std::uint8_t port[2] = { 0 };
				for (int j = 0; j < 4; j++)
					ip[j] = static_cast<uint8_t>(peer_ip[j]);
				for (int j = 0; j < 2; j++)
					port[j] = static_cast<uint8_t>(peer_port[j]);
				std::string ip_str = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
				std::uint32_t port_value = (port[0] << 8) | port[1];
				result.emplace_back(ip_str, port_value, "");
			}
			return result;
		}
		std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
		for (const auto& value : object) {
			const auto ip = value["ip"].get <std::string>();
			const auto port = value["port"].get <std::uint32_t>();
			const auto peer_id = string_to_hex(value["peer id"].get <std::string>());
			result.emplace_back(ip, port, peer_id);
		}
		return result;
	}

    std::string to_bencode(const json& j) {
		if (j.is_number()) {
			return "i" + std::to_string(j.template get<std::int64_t>()) + "e";
		}
		if (j.is_string()) {
            auto s = j.template get<std::string>();
			return std::to_string(s.length()) + ":" + s;
		}
		if (j.is_array()) {
			std::string s = "l";
			for (auto& el : j.items())
				s += to_bencode(el.value());
			return s + "e";
		}
        std::string result = "d";
		for (auto& el : j.items()) {
            result += to_bencode(el.key());
			result += to_bencode(j[el.key()]);
		}
        return result + "e";
	}
	struct Torrent {
		std::string info_hash;
		std::string announce;
		ns::Info info;
		std::string peer_id;
		std::uint64_t port;
		std::uint64_t uploaded;
		std::uint64_t downloaded;
		std::uint64_t left;
		bool compact;
	};
	struct TrackerResponse
	{
		std::int32_t interval;
		std::string peers;
	};
	TrackerResponse discover_peers(Torrent* torrent) {
		char url_buffer[2048];
		//strncpy(url_buffer, torrent->announce.c_str(), torrent->announce.length());
		//std::cout << urlencode(torrent->info_hash) << std::endl;
		std::string url_encoded_hash = urlencode(torrent->info_hash);
		sprintf(
			url_buffer,
			"%s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&compact=1",
			torrent->announce.c_str(),
			url_encoded_hash.c_str(),
			torrent->peer_id.c_str(),
			torrent->port,
			torrent->uploaded,
			torrent->downloaded,
			torrent->left);
		//std::cout << "Request Url: " << url_buffer << "\n";
		try
		{
			int n = 0;
			http::Request request{ url_buffer };
			const auto response = request.send("GET");
			std::string str_response = std::string{ response.body.begin(), response.body.end() };
			json r = decode_bencoded_value(str_response, n);
			auto peers = get_peers(r["peers"], true);
			for (const auto& [ip, port, peer_id] : peers)
			{
				std::cout << ip << ":" << port << "\n";
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Request failed, error: " << e.what() << '\n';
		}

		return {  };
	}
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
		return 1;
	}
	std::string command = argv[1];
	if (command == "decode") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
			return 1;
		}
		// You can use print statements as follows for debugging, they'll be visible when running tests.
		//std::cout << "Logs from your program will appear here!" << std::endl;
        //Uncomment this block to pass the first stage
		std::string encoded_value = argv[2];
		int n = 0;
		json decoded_value = decode_bencoded_value(encoded_value, n);
		std::cout << decoded_value.dump() << std::endl;
	}
	else if (command == "info") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " info <torrent_file>" << std::endl;
			return 1;
		}
        int n = 0;
		std::string file_name = argv[2];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		json decoded_value = decode_bencoded_value(str, n);
		std::cout << "Tracker URL: " << decoded_value["announce"].template get<std::string>() << std::endl;
		std::cout << "Length: " << decoded_value["info"]["length"].dump() << std::endl;
		ns::Info info_struct;
		ns::from_json(decoded_value["info"], info_struct);
		std::string bencoded_info = ns::to_bencode(decoded_value["info"]);
		SHA1 checksum;
		checksum.update(bencoded_info);
		const std::string hash = checksum.final();
		std::cout << "Info Hash: " << hash << std::endl;
		std::cout << "Piece Length: " << info_struct.piece_length << std::endl;
        std::cout << "Piece Hashes:" << info_struct.piece_length << std::endl;
		for (std::size_t i = 0; i < info_struct.pieces.length(); i += 20) {
			std::string piece = info_struct.pieces.substr(i, 20);
			std::stringstream ss;
			for (unsigned char byte : piece) {
				ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
			}
			std::cout << ss.str() << std::endl;
		}
        std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 9); // distribution in range [1, 6]
		std::string peer_id;
		for (int i = 0; i < 20; i++)
		{
			peer_id += std::to_string(dist6(dev));
		}
		std::cout << "Peer Id: " << peer_id << std::endl;
		ns::Torrent torrent;
		torrent.info_hash = hash;
		torrent.announce = decoded_value["announce"].template get<std::string>();
		torrent.info = info_struct;
		torrent.peer_id = peer_id;
		torrent.port = 6881;
		torrent.uploaded = 0;
		torrent.downloaded = 0;
		torrent.left = torrent.info.length;
		torrent.compact = 1;
		discover_peers(&torrent);
	}
	else if (command == "peers") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " info <torrent_file>" << std::endl;
			return 1;
		}
		int n = 0;
		std::string file_name = argv[2];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		json decoded_value = decode_bencoded_value(str, n);
		//std::cout << "Tracker URL: " << decoded_value["announce"].template get<std::string>() << std::endl;
		//std::cout << "Length: " << decoded_value["info"]["length"].dump() << std::endl;
		ns::Info info_struct;
		ns::from_json(decoded_value["info"], info_struct);
		std::string bencoded_info = ns::to_bencode(decoded_value["info"]);
		SHA1 checksum;
		checksum.update(bencoded_info);
		const std::string hash = checksum.final();
		//std::cout << "Info Hash: " << hash << std::endl;
		//std::cout << "Piece Length: " << info_struct.piece_length << std::endl;
		//std::cout << "Piece Hashes:" << info_struct.piece_length << std::endl;
		//for (std::size_t i = 0; i < info_struct.pieces.length(); i += 20) {
		//	std::string piece = info_struct.pieces.substr(i, 20);
		//	std::stringstream ss;
		//	for (unsigned char byte : piece) {
		//		ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
		//	}
		//	std::cout << ss.str() << std::endl;
		//}
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 9); // distribution in range [1, 6]
		std::string peer_id;
		for (int i = 0; i < 20; i++)
		{
			peer_id += std::to_string(dist6(dev));
		}
		//std::cout << "Peer Id: " << peer_id << std::endl;
		ns::Torrent torrent;
		torrent.info_hash = hash;
		torrent.announce = decoded_value["announce"].template get<std::string>();
		torrent.info = info_struct;
		torrent.peer_id = peer_id;
		torrent.port = 6881;
		torrent.uploaded = 0;
		torrent.downloaded = 0;
		torrent.left = torrent.info.length;
		torrent.compact = 1;
		discover_peers(&torrent);
	}
	else {
		std::cerr << "unknown command: " << command << std::endl;
		return 1;
	}
	return 0;
}