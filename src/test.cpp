#include <iostream>
#include <string>
#include <math.h>

using namespace std;

int main() { 
    string encoded_value = "l5:helloi52e";
    string ans = "[";
    size_t found_element = encoded_value.find_first_of(":i");
    
    while(found_element != string::npos) { 
        if(encoded_value[found_element] == ':') { // found a string element
            // get string's length
            int64_t string_number = encoded_value[found_element - 1] - 48;
            for(int i = found_element - 2; i >= 0; --i) { 
                if(isdigit(encoded_value[i])) { 
                    string_number += (encoded_value[i] - 48) * 10;
                }
            }
            // get the string
            string str = "\"";
            for(int i = found_element + 1; i <= found_element + string_number; ++i) { 
                str += encoded_value[i];
            }
            str += '\"';
            ans += str;
            ans += ',';
        }   
        else if(encoded_value[found_element] == 'i') { 
            // find trailing 'e'
            size_t find_e = encoded_value.find('e', found_element + 1);

            if(find_e != string::npos) { 
                // check if number is negative
                bool isNegative = encoded_value[found_element + 1] == '-' ? true : false;

                int64_t number_length = isNegative ?  (find_e - found_element - 2) : (find_e - found_element - 1);
                int64_t number = isNegative ? stoll(encoded_value.substr(found_element + 2, number_length)) : stoll(encoded_value.substr(found_element + 1, number_length));

                if((int)(log10(number)) + 1 < number_length) { // there is '0' in the start of number. Ex: i003e
                    number = 0;
                }

                isNegative ? number *= -1 : 0;
                ans += to_string(number);
                ans += ',';
            }
        }

        found_element = encoded_value.find_first_of(":i", found_element + 1);
    }

    ans.erase(ans.size() - 1);
    ans += ']';

    cout << ans;
}