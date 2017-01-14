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
      
  };

} /* namespace helper */


#endif /* __HELPER_H__ */
