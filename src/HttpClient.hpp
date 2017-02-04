/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

#include "EasySocket.hpp"
#include "HttpHeader.hpp"
#include <exception>
#include <map>
#include <fstream>

namespace net {
  namespace http {

    class HttpClientException: public std::exception {
      public:
	HttpClientException(std::string msg) : _msg(msg) { }
	virtual ~HttpClientException() = default;

	virtual const char* what() const throw() { return _msg.c_str(); }
      private:
	std::string _msg;
    };


    struct HttpClientConnect {
        /**
	 * @param host The host value.
	 * @param method The HTTP method to use.
	 * @param ssl Test if we need to use the SSL sockets.
	 * @param gzip Test if we need to use GZIP contents.
	 * @param headers Possible user defined headers.
	 * @param cookies Possible user defined cookies.
	 * @param params Possible user defined params.
	 * @param is_params an input stream to the params (if open).
	 * @param urlencode URL encode the parameters.
	 * @param uexcept Ignore some char for URL encode.
	 * @param isform Is form url encoded.
	 * @param print_query Print the query.
	 * @param print_chunk Print the chunk info.
	 * @param print_raw_resp Print the whole response.
	 * @param print_hex Prints the response in hex format
	 * @param multiparts Possible user defined headers for multipart.
	 */
	std::string host;
	std::string method;
	bool ssl;
	bool gzip;
	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> multiparts;
	std::vector<std::string> cookies;
	std::map<std::string, std::string> params;
	std::ifstream *is_params;
	bool urlencode;
	std::string uexcept;
	bool isform;
	bool print_query;
	bool print_chunk;
	bool print_raw_resp;
	bool print_nothing;
	bool print_hex;
    };

    class HttpClient {
      public:
	HttpClient(const std::string& appname);
	virtual ~HttpClient();
	
	/**
	 * @brief Get the plain text.
	 * @return std::string
	 */
	auto getPlainText() -> std::string&;

	/**
	 * @brief Get the HTTP header (from the response)
	 * @return HttpHeader
	 */
	auto getHttpHeader() -> HttpHeader&;

	/**
	 * @brief Connect the socket.
	 * @param connect The connect context
	 */
	auto connect(const HttpClientConnect& connect) -> void;

	/**
	 * @brief Test if the SSL is set.
	 */
	auto ssl() -> bool;

      private:
	std::string _appname;
	EasySocket _socket;
	std::string _host;
	bool _gzip;
	int _port;
	std::string _page;
	std::vector<char> _content;
	HttpHeader _hdr;
	std::string _plain;
	HttpClientConnect _connect;

	/**
	 * @brief Build the query request.
	 * @return The query.
	 */
	auto makeQuery() -> std::string;

	/**
	 * @brief Build the content type header.
	 * @param content The query contain content.
	 * @param isGet Is get method?
	 * @return The content type.
	 */
	auto makeContentType(bool content, bool isGET) -> std::string;
    };

  } /* namespace http */
} /* namespace net */
#endif /* __HTTPCLIENT_H__ */
