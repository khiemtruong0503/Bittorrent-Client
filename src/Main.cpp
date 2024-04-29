#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream> 
#include <sstream>
#include <utility>
#include <random>
#include <tuple>

#include "lib/sha1.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/HTTPRequest.hpp"

using json = nlohmann::json;

// json decode_bencoded_value(const std::string& encoded_value) {
//     size_t index = 0; // start index for parsing
//     if (std::isdigit(encoded_value[0])) { // string "5:Hello" -> "Hello"
//         // Example: "5:hello" -> "hello"
//         size_t colon_index = encoded_value.find(':');
//         if (colon_index != std::string::npos) {
//             std::string number_string = encoded_value.substr(0, colon_index);
//             int64_t number = std::atoll(number_string.c_str());
//             std::string str = encoded_value.substr(colon_index + 1, number);
//             return json(str);
//         } else {
//             throw std::runtime_error("Invalid encoded value: " + encoded_value);
//         }
//     }
//     else if (encoded_value[0] == 'i') { // integer "i52e" -> 52
//         size_t e_index = encoded_value.find('e');
//         if(e_index != std::string::npos) { 
//             int64_t integer_number = std::stoll(encoded_value.substr(1, e_index - 1));
//             return json(integer_number);
//         }
//     } 
//     else if (encoded_value[0] == 'l') { // list "l5:helloi52e" -> ["hello", 52]
//         std::string str = "[";
//         size_t found_element = encoded_value.find_first_of(":i");

//         while(found_element != std::string::npos) { 
//             if(encoded_value[found_element] == ':') { // found a string element
//                 //? get string's length
//                 int64_t number_string = encoded_value[found_element - 1] - 48;
//                 for(int i = found_element - 2; i >= 0; --i) { 
//                     if(isdigit(encoded_value[i])) { 
//                         number_string += (encoded_value[i] - 48) * 10;
//                     } 
//                 }
//                 //? get the string
//                 std::string s = "\"";
//                 for(int i = found_element + 1; i <= found_element + number_string; ++i) { 
//                     s += encoded_value[i];
//                 }
//                 s += '\"';
//                 if(s.size() > 2) { 
//                     str += s;
//                 }
//                 str += ',';
//             }
//             else if(encoded_value[found_element] == 'i'){ // found an integer element 
//                 //? find trailing 'e'
//                 size_t find_e = encoded_value.find('e', found_element + 1);

//                 if(find_e != std::string::npos) { 
//                     //? check if number is negative
//                     bool isNegative = encoded_value[found_element + 1] == '-' ? true : false;

//                     int64_t number_length = isNegative ? (find_e - found_element - 2) : (find_e - found_element - 1);
//                     int64_t number = isNegative ? stoll(encoded_value.substr(found_element + 2, number_length)) : stoll(encoded_value.substr(found_element + 1, number_length));

//                     if((int)(log10(number)) + 1 < number_length) { // there is leading '0' in the number. Ex: i003e
//                         number = 0;
//                     }

//                     isNegative ? number *= -1 : 0;
//                     str += std::to_string(number);
//                     str += ',';
//                 }
//             }

//             found_element = encoded_value.find_first_of(":i", found_element + 1);
//         }
        
//         if(str[str.size() - 1] == ',') { 
//             str.erase(str.size() - 1); // erase the last comma ','
//         }
//         str += "]";

//         return json(str);
//     }
//     else {
//         throw std::runtime_error("Unhandled encoded value: " + encoded_value);
//     }
// }

json decode_integer(const std::string &, size_t &);
json decode_string(const std::string &, size_t &);
json decode_list(const std::string &, size_t &);
json decode_dictionary(const std::string &, size_t &);
json decode_value(const std::string &, size_t &);
std::string json_to_bencode(const json &);


json decode_integer(const std::string& encoded_value, size_t &index) {
    size_t start = index + 1;
    size_t end = encoded_value.find('e', start);
    if (end == std::string::npos) { 
        throw std::runtime_error("Invalid integer encoding.");
    }
    else { 
        // i52e -> 52 or i-52e -> -52
        std::string str = encoded_value.substr(start, end);
        index = end + 1;
        return json(std::stoll(str));
    }
}

json decode_string(const std::string& encoded_value, size_t &index) { 
    // Ex: 5:Hello -> Hello
    size_t colon_index = encoded_value.find(':', index);
    if (colon_index != std::string::npos) { 
        std::string number_string = encoded_value.substr(index, colon_index - index);
        int64_t number = std::atoll(number_string.c_str());
        std::string str = encoded_value.substr(colon_index + 1, number);

        index = colon_index + 1 + number;
        return json(str);
    } else { 
        throw std::runtime_error("Invalid encoded value: " + encoded_value);
    }
}

json decode_list(const std::string& encoded_value, size_t &index) { 
    index++; // skip 'l'
    std::vector<json> list;
    while (index < encoded_value.size() && encoded_value[index] != 'e') { 
        if (encoded_value[index] == 'i') { 
            list.push_back(decode_integer(encoded_value, index));
        } else if (std::isdigit(encoded_value[index])) { 
            list.push_back(decode_string(encoded_value, index));
        } else if (encoded_value[index] == 'l') { 
            list.push_back(decode_list(encoded_value, index));
        }
    }
    index++; // skip 'e'
    return json(list);
}

json decode_dictionary(const std::string &encoded_value, size_t &index) { 
    index++; // skip 'd'
    json dict = json::object();
    while (index < encoded_value.length() && encoded_value[index] != 'e') { 
        json key = decode_string(encoded_value, index);
        json value = decode_value(encoded_value, index);
        dict[key.get<std::string>()] = value;
    }
    index++; // skip 'e'
    return dict;
}

json decode_value(const std::string &encoded_value, size_t &index) { 
    char type = encoded_value[index];
    switch (type) { 
        case 'i':
            return decode_integer(encoded_value, index);
        case 'l': 
            return decode_list(encoded_value, index);
        case 'd': 
            return decode_dictionary(encoded_value, index);
        default:
            if (std::isdigit(type)) { 
                return decode_string(encoded_value, index);
            } else { 
                throw std::runtime_error("Invalid encoded value.");
            }
    }
}

json decode_bencoded_value(const std::string& encoded_value) { 
    size_t index = 0; // start index for parsing
    return decode_value(encoded_value, index);
}

// read entire content of a file into a string
std::string read_file(const std::string &file_path) { 
    std::ifstream file(file_path, std::ios::binary);
    std::stringstream buff;
    if(file) { 
        buff << file.rdbuf();
        file.close();
        return buff.str();
    } else { 
        throw std::runtime_error("Failed to open file: " + file_path);
    }
}

// Function to convert binary data to a hexadecimal string
std::string to_hex_string(const std::string &binary) { 
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned char byte : binary) { 
        hex << std::setw(2) << static_cast<int>(byte);
    }
    return hex.str();
}
// parse torrent file, return tracker_url and length
void parse_torrent(const std::string &file_path) { 
    std::string content = read_file(file_path);
    json decoded_torrent = decode_bencoded_value(content);

    std::string tracker_url = decoded_torrent["announce"];
    int length = decoded_torrent["info"]["length"];

    std::cout << "Tracker URL: " << tracker_url << std::endl;
    std::cout << "Length: " << length << std::endl;

    std::string bencoded_info = json_to_bencode(decoded_torrent["info"]);
    SHA1 sha1;
    sha1.update(bencoded_info);
    std::string info_hash = sha1.final();
    
    int piece_length = decoded_torrent["info"]["piece length"];
    std::string piece_hashes = to_hex_string(decoded_torrent["info"]["pieces"]);
    std::cout << "Info Hash: " << info_hash << std::endl;
    std::cout << "Piece Length: " << piece_length << std::endl;
    std::cout << "Piece Hashes: " << piece_hashes << std::endl;
}

std::string json_to_bencode(const json &j) { 
    std::ostringstream os;
    if (j.is_object()) { 
        os << 'd';
        for (auto &el : j.items()) { 
            os << el.key().size() << ':' << el.key() << json_to_bencode(el.value());
        }
        os << 'e';
    } else if (j.is_array()) { 
        os << 'l';
        for (const json &item : j) { 
            os << json_to_bencode(item);
        }
        os << 'e';
    } else if (j.is_number_integer()) { 
        os << 'i' << j.get<int>() << 'e';
    } else if (j.is_string()) { 
        const std::string &value = j.get<std::string>();
        os << value.size() << ':' << value;
    }
    return os.str();
}

std::vector<unsigned char> HexToBytes(const std::string &hex) { 
    std::vector<unsigned char> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) { 
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::string url_encode(const std::string &p) { 
    std::vector<unsigned char> s(HexToBytes(p));
    static const char lookup[] = "0123456789abcdef";
    std::stringstream e;
    for(int i = 0, ix = s.size(); i < ix; ++i) { 
        const char &c = s[i];
        if ((48 <= c && c <= 57) || // 0-9
            (65 <= c && c <= 90) || // abc...xyz
            (97 <= c && c <= 122) || // ABC...XYZ
            (c == '-' || c == '_' || c == '.' || c == '~')
        ) { 
            e << c;
        } else { 
            e << '%';
            e << lookup[(c & 0xF0) >> 4];
            e << lookup[(c & 0x0F)];
        }
    }

    return e.str();
}

bool isASCII(const std::string &s) { 
    return !std::any_of(s.begin(), s.end(), [](char c) { 
        return static_cast<unsigned char>(c) > 127;
    });
}

namespace ns { 
    struct Info { 
        std::int64_t length;
        std::string name;
        std::int64_t piece_length;
        std::string pieces;
    };
};

struct Torrent { 
    std::string info_hash;
    std::string announce;
    ns::Info info;
    std::string peer_id;
    std::uint64_t port;
    std::uint64_t uploaded;
    std::uint64_t downloaded;
    std::uint64_t left;
    bool compacct;
};

struct TrackerResponse { 
    std::int32_t interval;
    std::string peers;
};

std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(json object, bool compact = false) { 
    if (compact) { 
        std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
        std::string peers = object.template get<std::string>();
        for(int i = 0; i < peers.length(); i += 6) { 
            std::string peer_ip = peers.substr(i, 4);
            std::string peer_port = peers.substr(i + 4, 2);

            std:uint8_t ip[4] = { 0 };
            std::uint8_t port[2] = { 0 };

            for(int j = 0; j < 4; ++j) { 
                ip[j] = static_cast<uint8_t>(peer_ip[j]);
            }

            for(int j = 0; j < 2; ++j) { 
                port[j] = static_cast<uint8_t>(peer_port[j]);
            }

            std::string ip_str = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
            std::uint32_t port_value = (port[0] << 8) | port[1];
            result.emplace_back(ip_str, port_value, "");
        }

        return result;
    }

    std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
    for(const auto &value : object) { 
        const auto ip = value["ip"].get <std::string>();
        const auto port = value["port"].get <std::uint32_t>();
        const auto peer_id = to_hex_string(value["peer id"].get <std::string>());
        result.emplace_back(ip, port, peer_id);
    }

    return result;
}

TrackerResponse discover_peers(Torrent *torrent) { 
    char url_buffer[2048];
    std::string url_encoded_hash = url_encode(torrent->info_hash);
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

        try { 
            http::Request request{ url_buffer };
            const auto response = request.send("GET");
            std::string str_response = std::string{ response.body.begin(), response.body.end()};
            json r = decode_bencoded_value(str_response);
            auto peers = get_peers(r["peers"], true);
            for(const auto& [ip, port, peer_id] : peers) { 
                std::cout << ip << ":" << port << "\n";
            }
        }
        catch (const std::exception &e) { 
            std::cerr << "Request failed, error: " << e.what() << '\n';
        }

        return { };
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
        // std::cout << "Logs from your program will appear here!" << std::endl;

        // Uncomment this block to pass the first stage
        std::string encoded_value = argv[2];
        json decoded_value = decode_bencoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else if (command == "info") { 
        if (argc < 3) { 
            std::cerr << "Usage: " << argv[0] << " info <torrent_file> " << std::endl;
            return 1;
        }
        try { 
            std::string file_path = argv[2];
            parse_torrent(file_path);
        } catch (const std::exception &e) { 
            std::cerr << "Error getting info: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
