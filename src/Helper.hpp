/**
*******************************************************************************
* <p><b>Project hcli</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#ifndef __HELPER_H__
#define __HELPER_H__


#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include <cctype>
#include <iomanip>
#include <iterator>

#include <iostream>
namespace helper {

  using vstring = std::vector<std::string>;

  class Helper {
    public:
      Helper() = default;
      virtual ~Helper() = default;

      /**
       * @brief Split a string into a vector.
       * @param s The string to split.
       * @param delim The delimitor.
       * @return vector<string>
       */
      static auto split(const std::string& s, const char delim = ' ') -> vstring {
	vstring elems;
	std::istringstream ss((std::string(s)));
	std::string item;
	while (std::getline(ss, item, delim))
	  elems.push_back(item);
	return elems;
      }

      /**
       * @brief Trim a string.
       * @param str The string to trim.
       * @param delim The delimitor.
       * @return the trimed string.
       */
      static auto trim(const std::string& str, char delim = ' ') -> std::string {
	std::string s = str; // duplicate input for manipulation
	std::string::size_type pos = s.find_last_not_of(delim);
	if(pos != std::string::npos) {
	  s.erase(pos + 1);
	  pos = s.find_first_not_of(delim);
	  if(pos != std::string::npos) s.erase(0, pos);
	}
	else s.erase(s.begin(), s.end());
	return s;
      }

      static auto http_label(bool ssl) -> std::string {
	return std::string("http") + std::string((ssl ? "s://" : "://"));
      }

      /**
       * @brief Encode an URL
       * @param value The URL to encode.
       * @return the endoded URL.
       */
      static auto urlEncode(const std::string &value, const std::string &except = "") -> std::string{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
	  std::string::value_type c = (*i);

	  // Keep alphanumeric and other accepted characters intact
	  if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || except.find(c) != std::string::npos) {
            escaped << c;
            continue;
	  }

	  // Any other characters are percent-encoded
	  escaped << std::uppercase;
	  escaped << '%' << std::setw(2) << int((unsigned char) c);
	  escaped << std::nouppercase;
	}	
	return escaped.str();
      }


      static auto toCharVector(const std::string& str) -> std::vector<char> {
	return std::vector<char>(str.begin(), str.end());
      }

      static auto fromCharVector(std::vector<char> data) -> std::string {
	return std::string(data.begin(), data.end());
      }
      
  };

} /* namespace helper */


#endif /* __HELPER_H__ */
