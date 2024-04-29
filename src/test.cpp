#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

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
        std::string number_string = encoded_value.substr(index, colon_index - 1);
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
json decode_bencoded_value(const std::string& encoded_value, size_t &index) { 
    if (encoded_value[index] == 'i') { 
        return decode_integer(encoded_value, index);
    } else if (isdigit(encoded_value[index])) { 
        return decode_string(encoded_value, index);
    } else if (encoded_value[index] == 'l') { 
        return decode_list(encoded_value, index);
    } else { 
        throw std::runtime_error("Invalid encodede value.");
    }
}

json decode_bencoded_value(const std::string& encoded_value) { 
    size_t index = 0; // start index for parsing
    return decode_bencoded_value(encoded_value, index);
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
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
