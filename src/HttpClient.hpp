/**
*******************************************************************************
* <p><b>Project hcli</b><br/>
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
	 * @param host The host value.
	 * @param method The HTTP method to use.
	 * @param ssl Test if we need to use the SSL sockets.
	 * @param gzip Test if we need to use GZIP contents.
	 * @param headers Possible user defined headers.
	 * @param cookies Possible user defined cookies.
	 * @param params Possible user defined params.
	 */
	auto connect(std::string host, std::string method, bool ssl, bool gzip, std::map<std::string, std::string> headers, std::vector<std::string> cookies, std::map<std::string, std::string> params) -> void;

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
	std::string _method;
	std::map<std::string, std::string> _headers;
	std::map<std::string, std::string> _params;
	std::vector<std::string> _cookies;
	std::string _sparams;
	HttpHeader _hdr;
	std::string _plain;
	/**
	 * @brief Deflate plain data in GZIP.
	 * @param toDeflate To deflate.
	 * @return The deflated data.
	 */
	static auto deflate(const std::string toDeflate) -> std::string;

	/**
	 * @brief Inflate GZIP data in plain text.
	 * @param toInflate inflate.
	 * @return The inflated data.
	 */
	static auto inflate(const std::string toInflate) -> std::string;

	/**
	 * @brief Build the query request.
	 * @return The query.
	 */
	auto makeQuery() -> std::string;
    };

  } /* namespace http */
} /* namespace net */
#endif /* __HTTPCLIENT_H__ */
