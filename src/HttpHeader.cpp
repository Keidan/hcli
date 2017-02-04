/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#include "HttpHeader.hpp"
#include <algorithm>
#include <iterator>
#include <iostream>

namespace net {
  namespace http {

    using std::istringstream;
    using std::vector;
    using std::string;
    using std::size_t;
    using std::istream_iterator;
    using helper::vstring;
    using helper::Helper;

    HttpHeader::HttpHeader() : _done(false), _code(0), _reason(""), _content(), _hdrs(), _length(0) {
    }

    HttpHeader::~HttpHeader() {
      clear();
    }

    /**
     * @brief Clear the headers contexts.
     */
    auto HttpHeader::clear() -> void {
      _done = false;
      _code = 0;
      _length = 0;
      _reason = "";
      _content.str("");
      _hdrs.clear();
    }

    /**
     * @brief Test if all the headers are sent.
     * @return true if done.
     */
    auto HttpHeader::done() -> bool {
      return _done;
    }

    /**
     * @biref Get the response code.
     * @return The code.
     */
    auto HttpHeader::code() -> unsigned int {
      return _code;
    }

    /**
     * @brief Get the header length.
     * @return length
     */
    auto HttpHeader::length() -> std::size_t {
      return _length;
    }

    /**
     * @biref Get the response reason.
     * @return The reason.
     */
    auto HttpHeader::reason() -> string {
      return _reason;
    }

    /**
     * @brief Append data to decode.
     * @param data The data.
     */
    auto HttpHeader::append(const string& data) -> void {
      if(!_done) {
	_content << data;
	string s = _content.str();
	size_t found = s.find("\r\n\r\n");
	/* get all the headers */
	if (found!=string::npos) {
	  _done = true;
	  /* only preserve the headers */
	  s = s.substr(0, found);
	  _length = s.size() + 4;
	  /* prepare for header parsing */
	  vstring tokens = Helper::split(s, '\n');
	  for(vstring::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
	    string line = *it;
	    /* extract the code and the reason */
	    if(_code == 0 && (found = line.find("HTTP/")) != string::npos) {
	      line = line.substr(found + 9);
	      found = line.find(" ");
	      if(found != string::npos) {
		_code = std::atoi(line.substr(0, found).c_str());
		_reason = Helper::trim(Helper::trim(line.substr(found+1)), '\r');
	      }
	      /* now extract the headers */
	    } else if(_code != 0) {
	      found = line.find(":");
	      if(found != string::npos) {
		string key = line.substr(0, found);
		string val = Helper::trim(Helper::trim(line.substr(found+1)), '\r');
		auto it = find(key);
		if(it != _hdrs.end()) {
		  //std::cout << "Update key '" << key << "' with value '" << val << "'" << std::endl;
		  it->second.push_back(val);
		}
		else {
		  vstring vs;
		  vs.push_back(val);
		  //std::cout << "Add key '" << key << "' with value '" << val << "'" << std::endl;
		  _hdrs.push_back(std::make_pair(key, vs));
		}
	      }
	    }
	  }
	}
      }
    }
      

    /**
     * @brief Find an entry from the key.
     * @param key The key to find.
     * @return The value.
     */
    auto HttpHeader::find(const string& key) -> HttpHeadersIterator {
      auto it = std::find_if(_hdrs.begin(), _hdrs.end(),
			     [&key](const HttpHeaderItem& element){ return element.first == key;} );
      return it;
    }

    /**
     * @brief Test if the key is present or not.
     * @param key The header key.
     * @return true if the key is present.
     */
    auto HttpHeader::contains(const string& key) -> bool {
      return find(key) != _hdrs.end();
    }

    /**
     * @brief Test if the key is present and if the value match.
     * @param key The header key.
     * @param value The header value.
     * @return true if the key is present and match.
     */
    auto HttpHeader::equals(const std::string& key, const std::string& value) -> bool {
      auto it = find(key);
      if(it != _hdrs.end()) {
	if(std::find(it->second.begin(), it->second.end(), value) != it->second.end())
	  return true;
      }
      return false;
    }

    /**
     * @brief Get the keys list.
     * @return The list of keys.
     */
    auto HttpHeader::keys() -> vstring {
      vstring v;
      for(HttpHeadersConstIterator it = _hdrs.begin(); it != _hdrs.end(); ++it) {
	string i = it->first;
	v.push_back(i);
      }
      return v;
    }

    /**
     * @brief Get the specific header value.
     * @param key The header key.
     * @return The value.
     */
    auto HttpHeader::get(const string& key) -> vstring {
      vstring v;
      auto it = find(key);
      if(it != _hdrs.end())
	v = it->second;
      return v;
    }

  } /* namespace http */
} /* namespace net */
