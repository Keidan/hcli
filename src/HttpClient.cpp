/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#include "HttpClient.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <map>
#include <vector>
#include "Helper.hpp"
#include "GZIP.hpp"

namespace net {
  namespace http {

    using std::cout;
    using std::endl;
    using std::map;
    using std::vector;
    using std::string;
    using std::size_t;
    using std::stringstream;
    using std::ifstream;
    using net::EasySocket;
    using net::http::HttpHeader;
    using helper::Helper;
    using utils::GZIP;
    using utils::GZIPMethod;

    constexpr size_t windowBits = 15;
    constexpr size_t GZIP_ENCODING = 16;
    constexpr size_t CHUNK = 1024;


    #define addDefaultHeader(oss, name, value) do {			\
      if(_connect.headers.find(name) == _connect.headers.end())		\
	oss << name << ": " << value << "\r\n";				\
    } while(0)

    HttpClient::HttpClient(const string& appname) : _appname(appname), _socket(), 
						     _port(80), _page("/"), 
                                                     _content(), _hdr(), _plain(""), _connect(), 
                                                     _boundary(_appname + Helper::generateHexString(16)) {
    }
    HttpClient::~HttpClient() {
      _socket.disconnect();
    }

    /**
     * @brief Test if the SSL is set.
     */
    auto HttpClient::ssl() -> bool {
      return _socket.ssl();
    }
    /**
     * @brief Build the content type header.
     * @param content The query contain content.
     * @param isGet Is get method?
     * @return The content type.
     */
    auto HttpClient::makeContentType(bool content, bool isGET) -> std::string {
      std::ostringstream oss;
      if(!_connect.multiparts.empty())
        addDefaultHeader(oss, "Content-Type", "multipart/form-data; boundary=---------------------------" << _boundary);      
      else if(_connect.isform)
	addDefaultHeader(oss, "Content-Type", "application/x-www-form-urlencoded");
      else if(_connect.gzip)
	addDefaultHeader(oss, "Content-Type", "application/javascript");
      else if(content && !isGET) {
	addDefaultHeader(oss, "Content-Type", "text/html");
      }
      return oss.str();
    }
 
    /**
     * @brief Build the query request.
     * @return The query.
     */
    auto HttpClient::makeQuery() -> string {
      bool isGET = (_connect.method == "GET");
      string h = _connect.host;
      string content =  Helper::fromCharVector(_content);
      bool deflate = false;
      bool gzip = false;
      // if(h.back() != '/') h += "/";
      std::ostringstream oss;
      oss << _connect.method << " " << Helper::http_label(_socket.ssl()) << h <<  _page;
      if(isGET) oss << _connect.urlencode ? Helper::urlEncode(content) : content;
      oss << " HTTP/1.1\r\n";
      addDefaultHeader(oss, "Host",  h + (_port != 80 ? ":"+std::to_string(_port) : ""));
      addDefaultHeader(oss, "User-Agent", _appname);
      addDefaultHeader(oss, "Accept", "application/json,text/javascript,text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
      addDefaultHeader(oss, "Accept-Language", "q=0.8,en-US;q=0.5,en;q=0.3");
      for(map<string, string>::const_iterator it = _connect.headers.begin(); it != _connect.headers.end(); ++it) {
	oss << it->first << ": " << it->second << "\r\n";
        if(it->first == "Accept-Encoding") {
          gzip = (it->second.find("gzip") != string::npos);
          deflate = (it->second.find("deflate") != string::npos);
        }
      }
      if(_connect.gzip && !deflate && !gzip)
        deflate = true;
      
      for(vector<string>::const_iterator it = _connect.cookies.begin(); it != _connect.cookies.end(); ++it)
	oss << "Cookie: " << (*it) << "\r\n";

      oss << makeContentType(!content.empty(), isGET); 
      string body;
      if(gzip)
        body = GZIP::compress(content, GZIPMethod::GZ);
      else if(deflate)
        body = GZIP::compress(content, GZIPMethod::DEFLATE);
      else if(_connect.urlencode) {
        body = Helper::urlEncode(content, _connect.uexcept);
      }
      else 
        body = content;

      std::stringstream ss_boundary;
      size_t extra_length = 0;
      ss_boundary.str("");
      if(!_connect.multiparts.empty()) {
        ss_boundary << "-----------------------------" << _boundary << "\r\n";
        for(map<string, string>::const_iterator it = _connect.multiparts.begin(); it != _connect.multiparts.end(); ++it)
          ss_boundary << it->first << ": " << it->second << "\r\n";
        ss_boundary << "\r\n";
        ss_boundary << body << "\r\n";
        std::string footer = "-----------------------------" + _boundary + "--\r\n\r\n";
        ss_boundary << footer;
        extra_length = footer.size() + 2;
      }


      if(gzip || deflate) {
	addDefaultHeader(oss, "Accept-Encoding", "gzip, deflate");
	if(!content.empty() && !isGET)
	  addDefaultHeader(oss, "Content-Encoding", std::string(gzip ? "gzip" : "deflate"));
      }
      if(!content.empty() && !isGET)
	addDefaultHeader(oss, "Content-Length", std::to_string(body.size() + extra_length));
      addDefaultHeader(oss, "Connection", "close\r\n");
      if(!_connect.multiparts.empty()) oss << ss_boundary.str();
      else {
        if(!content.empty() && !isGET)
	  oss << body;
	else
	  oss << "\r\n";
      }
      return oss.str();
    }

    /**
     * @brief Connect the socket.
     * @param connect The connect context
     */
    auto HttpClient::connect(const HttpClientConnect& connect) -> void {
      _connect = connect;
      bool isGET = (_connect.method == "GET");
      if(_connect.host.empty()) {
	throw HttpClientException("Unable to start the client without host!");
      }
      _socket.ssl(_connect.ssl);
      /* decode the host value */
      _port = !_socket.ssl() ? 80 : 443;
      _page = "/";
      /* get the page value */
      size_t found = _connect.host.find("/");
      if(found != string::npos) {
	_page = _connect.host.substr(found);
	_connect.host = _connect.host.substr(0, found);
      }
      /* get the port value */
      found = _connect.host.find(":");
      if(found != string::npos) {
	_port = std::atoi(_connect.host.substr(found + 1).c_str());
	_connect.host = _connect.host.substr(0, found);
      }
      if(_connect.is_params->is_open()) {
	std::streamsize size = _connect.is_params->tellg();
	_connect.is_params->seekg(0, std::ios::beg);
	_content.clear();
	_content.resize(size);
	_connect.is_params->read(_content.data(), size);
      } else {
	string s;
	if(!_connect.params.empty()) {
	  s = isGET ? "?" : "";
	  for(map<string, string>::const_iterator it = _connect.params.begin(); it != _connect.params.end(); ++it)
	    s += it->first + "=" + it->second + "&";
	  if(s.at(s.size() - 1) == '&') s.erase(s.size() - 1);
	  _content = Helper::toCharVector(s);
	}
      }
      string content = Helper::fromCharVector(_content);
      cout << "Use location: " << Helper::http_label(_connect.ssl) << _connect.host << (isGET && !_content.empty() ? content : "") << endl; 
      cout << "Query " << _connect.method << " " << Helper::http_label(_connect.ssl) << _connect.host << _page << endl;
      if(!_content.empty()) cout << "Whith content " << Helper::toHumanStringSize(content.size()) << endl;
      /* establishes a connection with the remote host */
      _socket.connect(_connect.host, _port);
      /* Send the request */
      string output = makeQuery();
      if(_connect.print_query)
	cout << "Query: " << endl << "***" << endl << output << endl << "***" << endl;
      _socket << output;
      cout << "Wait for response ..." << endl;
      /* wait a second for processing. */
      sleep(1);

      std::ostringstream oss;
      for(;;) {
	/* store the response and build headers list */
	string readdata;
	_socket >> readdata;
        if(readdata.empty()) break;
	if(!_hdr.done()) _hdr.append(readdata);
	oss << readdata;
	if(_connect.print_raw_resp)
	  cout << readdata << endl;
        if(_connect.print_hex) {
          cout << Helper::print_hex((unsigned char*)readdata.c_str(), readdata.size());
          cout << "\n";
        }
      }
      string str = oss.str();     
      string readdata = str.substr(_hdr.length());
      readdata = readdata.substr(0, readdata.size() - 2);

      // cout << "==" << readdata <<"==" << readdata.size()<< endl << endl;
      /* Test if the body response is chuncked */
      oss.str("");
      if(_hdr.equals("Transfer-Encoding", "chunked")) {
	if(_connect.print_chunk)
	  cout << "Chunked response ..." << endl;
	size_t chunk = 0, count = 0;
	do {
	  string w = readdata;
	  size_t found = w.find("\r\n");
	  if(found == 0) {
	    w = w.substr(2);
	    found = w.find("\r\n");
	    if(found != string::npos) found += 2;
	  }
	  if(found != string::npos) {
	    stringstream ss;
	    string v = Helper::trim(Helper::trim(Helper::trim(readdata.substr(0, found), '\r'), '\n'));
	    ss << std::hex << v;
	    ss >> chunk;
	    if(_connect.print_chunk)
	      cout << (++count) << " chunk bloc size " << chunk << " (0x" << v << ") - " << Helper::toHumanStringSize(chunk) << endl;
	    readdata = readdata.substr(found + 2);
            /*if(chunk > readdata.size()) {
              std::ostringstream er;
              er << "The chunk size (" << Helper::toHumanStringSize(chunk) << ") is greater than the response length (" << Helper::toHumanStringSize(readdata.size()) << ")";
              throw HttpClientException(er.str());
	    }*/
            if(!_connect.gzip) {

	      oss << readdata.substr(0, chunk);
	      readdata = readdata.substr(chunk);
            } else {
	      readdata = readdata.substr(0, readdata.size() - 3);
	      oss << readdata;
	    }
	  }
	} while(chunk && !_connect.gzip);
      } else
	oss << readdata;
      /* test support of gzip content */
      _plain = oss.str();
      //cout << Helper::print_hex((unsigned char*)_plain.c_str(), _plain.size());
      //cout << "\n";
      if(_hdr.equals("Content-Encoding", "gzip")) {
	string t = _plain;
        _plain = GZIP::decompress(t, GZIPMethod::GZ);
      } else if(_hdr.equals("Content-Encoding", "deflate")) {
	string t = _plain;
        _plain = GZIP::decompress(t, GZIPMethod::DEFLATE);
      }
    }

    /**
     * @brief Get the plain text.
     * @return string
     */
    auto HttpClient::getPlainText() -> string& {
      return _plain;
    }

    /**
     * @brief Get the HTTP header (from the response)
     * @return HttpHeader
     */
    auto HttpClient::getHttpHeader() -> HttpHeader& {
      return _hdr;
    }

  } /* namespace http */
} /* namespace net */
