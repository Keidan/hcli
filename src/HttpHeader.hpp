/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#ifndef __HTTPHEADER_H__
#define __HTTPHEADER_H__

#include "Helper.hpp"
#include <utility>

namespace net {
  namespace http {

    class HttpHeader {
      public:

	HttpHeader();
	~HttpHeader();

	using HttpHeaderItem = std::pair<std::string, helper::vstring>;
	using HttpHeaders = std::vector<HttpHeaderItem>;
	using HttpHeadersIterator = HttpHeaders::iterator;
	using HttpHeadersConstIterator = HttpHeaders::const_iterator;
      
	/**
	 * @brief Clear the headers contexts.
	 */
	auto clear() -> void;

	/**
	 * @brief Test if all the headers are sent.
	 * @return true if done.
	 */
	auto done() -> bool;

	/**
	 * @brief Append data to decode.
	 * @param data The data.
	 */
	auto append(const std::string& data) -> void;
      
	/**
	 * @brief Get the specific header value.
	 * @param key The header key.
	 * @return The value.
	 */
	auto get(const std::string& key) -> helper::vstring;

	/**
	 * @brief Test if the key is present or not.
	 * @param key The header key.
	 * @return true if the key is present.
	 */
	auto contains(const std::string& key) -> bool;

	/**
	 * @brief Test if the key is present and if the value match.
	 * @param key The header key.
	 * @param value The header value.
	 * @return true if the key is present and match.
	 */
	auto equals(const std::string& key, const std::string& value) -> bool;

	/**
	 * @brief Get the keys list.
	 * @return The list of keys.
	 */
	auto keys() -> helper::vstring;

	/**
	 * @biref Get the response code.
	 * @return The code.
	 */
	auto code() -> unsigned int;

	/**
	 * @biref Get the response reason.
	 * @return The reason.
	 */
	auto reason() -> std::string;

	/**
	 * @brief Get the header length.
	 * @return length
	 */
	auto length() -> std::size_t;


      private:
	bool _done;
	unsigned int _code;
	std::string _reason;
	std::stringstream _content;
	HttpHeaders _hdrs;
	std::size_t _length;

	/**
	 * @brief Find an entry from the key.
	 * @param key The key to find.
	 * @return The value.
	 */
	auto find(const std::string& key) -> HttpHeadersIterator;
    };

  } /* namespace http */
} /* namespace net */
#endif /* __HTTPHEADER_H__ */
