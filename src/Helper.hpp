/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
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
#include <cstdio>
#include <cstring>

namespace helper {

  using vstring = std::vector<std::string>;

  constexpr double SIZE_1KB = 0x400;
  constexpr double SIZE_1MB = 0x100000;
  constexpr double SIZE_1GB = 0x40000000;

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

      template<typename T>
      static std::string int_to_hex(T i) {
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(sizeof(T)*2) 
	       << std::hex << (unsigned int)i;
	return stream.str();
      }

      static auto print_hex(unsigned char* buffer, int len, bool print_raw = false) -> std::string {
	std::ostringstream oss;
	int i = 0, max = 16, loop = len;
	unsigned char *p = buffer;
	char line [max + 3]; /* spaces + \0 */
	memset(line, 0, sizeof(line));
	while(loop--) {
	  unsigned char c = *(p++);
	  if(!print_raw) {
	    oss << int_to_hex(c) << " ";
	    /* only the visibles char */
	    if(c >= 0x20 && c <= 0x7e) line[i] = c;
	    /* else mask with '.' */
	    else line[i] = 0x2e; /* . */
	  } else oss << int_to_hex(c);
	  
	  /* next line */
	  if(i == max) {
	    if(!print_raw) oss << "  " << line << "\n";
	    else oss << "\n";
	    /* re init */
	    i = 0;
	    memset(line, 0, sizeof(line));
	  }
	  /* next */
	  else i++;
	  /* add a space in the midline */
	  if(i == max / 2 && !print_raw) {
	    oss << " ";
	    line[i++] = 0x20;
	  }
	}
	/* align 'line'*/
	if(i != 0 && (i <= max || i <= len) && !print_raw) {
	  while(i++ <= max) oss << "   "; /* 3 spaces ex: "00 " */
	  oss << line;
	}
	return oss.str();
      }

      static auto toHumanStringSize(std::size_t size) -> std::string {
	std::ostringstream sf;
	double d = static_cast<double>(size);
	if(size < 1000)
	  sf << size << " byte" << (size ? "s" : "");
	else if(size < 1000000)
	  sf << std::setprecision(2) << (d/SIZE_1KB) << " Kb";
	else if(size < 1000000000)
	  sf << std::setprecision(2) << (d/SIZE_1MB) << " Mb";
	else
	  sf << std::setprecision(2) << (d/SIZE_1GB) << " Gb";
	return sf.str();
      }
      
  };

} /* namespace helper */


#endif /* __HELPER_H__ */
