/**
*******************************************************************************
* <p><b>Project hcli</b><br/>
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
#include <zlib.h>


namespace net {
  namespace http {

    using std::cout;
    using std::endl;
    using std::map;
    using std::vector;
    using std::string;
    using std::size_t;
    using std::stringstream;
    using net::EasySocket;
    using net::http::HttpHeader;
    using helper::Helper;


    constexpr size_t windowBits = 15;
    constexpr size_t GZIP_ENCODING = 16;
    constexpr size_t CHUNK = 1024;


    HttpClient::HttpClient(const string& appname) : _appname(appname), _socket(), _host(""), 
						     _gzip(false), _port(80), _page("/"), _method("GET"), 
						     _headers(), _params(), _cookies(), _sparams(""), _hdr(), _plain("") {
    }
    HttpClient::~HttpClient() {
      _socket.disconnect();
    }

    /**
     * @brief Deflate plain data in GZIP.
     * @param toDeflate To deflate.
     * @return The deflated data.
     */
    auto HttpClient::deflate(const string toDeflate) -> string {
      z_stream strm;
      strm.zalloc = Z_NULL;
      strm.zfree  = Z_NULL;
      strm.opaque = Z_NULL;
      if(::deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			       windowBits | GZIP_ENCODING, 8,
		       Z_DEFAULT_STRATEGY) < 0) {
	throw HttpClientException("Unable to load deflateInit2!");
      }
      strm.next_in = (unsigned char *) toDeflate.c_str();
      strm.avail_in = toDeflate.length();
      string out("");
      unsigned char buffer[CHUNK];
      do {
	bzero(buffer, CHUNK);
        strm.avail_out = CHUNK - 1;
        strm.next_out = buffer;
	int r;
        if((r = ::deflate (&strm, Z_FINISH)) < 0) {
	  throw HttpClientException("deflate error " + std::to_string(r));
	}
	out += string((char*)buffer);
      }
      while (strm.avail_out == 0);
      ::deflateEnd(&strm);
      return out;
    }

    /**
     * @brief Inflate GZIP data in plain text.
     * @param toInflate inflate.
     * @return The inflated data.
     */
    auto HttpClient::inflate(const string toInflate) -> string {
      const int buffersize = 16384;
      Bytef buffer[buffersize];
      std::string out("");
      z_stream cmpr_stream;
      cmpr_stream.next_in = (unsigned char *)toInflate.c_str();
      cmpr_stream.avail_in = toInflate.size();
      cmpr_stream.total_in = 0;
      cmpr_stream.next_out = buffer;
      cmpr_stream.avail_out = buffersize;
      cmpr_stream.total_out = 0;
      cmpr_stream.zalloc = Z_NULL;
      cmpr_stream.zalloc = Z_NULL;
      if(::inflateInit2(&cmpr_stream, -8 ) != Z_OK) {
	throw HttpClientException("Unable to load inflateInit2!");
      }
      do {
	int status = ::inflate( &cmpr_stream, Z_SYNC_FLUSH );

	if(status == Z_OK || status == Z_STREAM_END) {
	  out.append((char *)buffer, buffersize - cmpr_stream.avail_out);
	  cmpr_stream.next_out = buffer;
	  cmpr_stream.avail_out = buffersize;
	} else {
	  ::inflateEnd(&cmpr_stream);
	}

	if(status == Z_STREAM_END) {
	  ::inflateEnd(&cmpr_stream);
	  break;
	}
      }while(cmpr_stream.avail_out == 0);
      return out;
    }

    /**
     * @brief Test if the SSL is set.
     */
    auto HttpClient::ssl() -> bool {
      return _socket.ssl();
    }
 
    /**
     * @brief Build the query request.
     * @return The query.
     */
    auto HttpClient::makeQuery() -> string {
      string h = _host;
      if(h.back() != '/') h += "/";
      string output = _method + " " + _page;
      if(_method == "GET") output += _sparams;
      output += " HTTP/1.1\r\n";
      if(_headers.find("Host") == _headers.end())
	output += "Host: " + h + "\r\n";
      if(_headers.find("User-Agent") == _headers.end())
	output += "User-Agent: " + _appname + "\r\n";
      if(_headers.find("Accept") == _headers.end())
	output += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
      if(_headers.find("Accept-Language") == _headers.end())
	output += "Accept-Language: q=0.8,en-US;q=0.5,en;q=0.3\r\n";
      for(map<string, string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
	output += it->first + ": " + it->second + "\r\n";
	if(it->first == "Accept-Encoding" && it->second.find("gzip") != string::npos && !_gzip)
	  _gzip = true;
      }
      for(vector<string>::const_iterator it = _cookies.begin(); it != _cookies.end(); ++it)
	output += "Cookie: " + (*it) + "\r\n";
      if(_gzip && _headers.find("Accept-Encoding") == _headers.end())
	output += "Accept-Encoding: gzip, deflate\r\n";
      if(!_sparams.empty() && !(_method == "GET")) {
	string sp = _sparams;
	if(_headers.find("Content-Type") == _headers.end())
	  output += "Content-Type: text/html\r\n";
	if(_headers.find("Content-Length") == _headers.end())
	  output += "Content-Length: " + std::to_string(sp.length()+2) + "\r\n";
      };
      if(_headers.find("Connection") == _headers.end())
	output += "Connection: close\r\n\r\n";

      if(!_sparams.empty() && !(_method == "GET"))
	output += (_gzip ? deflate(_sparams) : _sparams);
      return output;
    }

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
    auto HttpClient::connect(string host, string method, bool ssl, bool gzip, map<string, string> headers, vector<string> cookies, map<string, string> params) -> void {
      _gzip = gzip;
      _headers = headers;
      _params = params;
      _cookies = cookies;
      _method = method;
      _host = host;
      if(_host.empty()) {
	throw HttpClientException("Unable to start the client without host!");
      }
      _socket.ssl(ssl);
      /* decode the host value */
      _port = !_socket.ssl() ? 80 : 443;
      _page = "/";
      /* get the page value */
      size_t found = _host.find("/");
      if(found != string::npos) {
	_page = _host.substr(found);
	_host = _host.substr(0, found);
      }
      /* get the port value */
      found = _host.find(":");
      if(found != string::npos) {
	_port = std::atoi(_host.substr(found + 1).c_str());
	_host = _host.substr(0, found);
      }
      _sparams = "";
      if(!_params.empty()) {
	_sparams = (_method == "GET") ? "?" : "";
	for(map<string, string>::const_iterator it = _params.begin(); it != _params.end(); ++it)
	  _sparams += it->first + "=" + it->second + "&";
	if(_sparams.at(_sparams.length() - 1) == '&') _sparams.erase(_sparams.size() - 1);
      }
      if(_method == "GET")
	cout << "Send query: " << _method << " " << _page << _sparams << " to host " << _host << " and port " << _port << endl;
      else
	cout << "Send query: " << _method << " " << _page << (_sparams.empty() ? "" : " whith content " + _sparams) << " to host " << _host << " and port " << _port << endl;
    
      cout << "Use location: http" << (!ssl ? "://" : "s://") << host << ((_method == "GET") && !_sparams.empty() ? _sparams : "") << endl; 
      _socket.connect(_host, _port);
      /* Send the request */
      string output = makeQuery();
      cout << "Query: " << endl << output << endl << endl;
      _socket << output;
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
	//cout << "==" << readdata <<"=="<< endl << endl;
      }
      string readdata = oss.str().substr(_hdr.length());
      readdata = readdata.substr(0, readdata.length() - 2);

      //cout << "==" << readdata <<"=="<< endl << endl;
      /* Test if the body response is chuncked */
      oss.str("");
      if(_hdr.equals("Transfer-Encoding", "chunked")) {
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
	    cout << (++count) << " chunk bloc length:" << chunk << " (0x" << v << ")" << endl;
	    readdata = readdata.substr(found + 2);
	    if(!_gzip) {
	      oss << readdata.substr(0, chunk);
	      readdata = readdata.substr(chunk);
	    } else {
	      readdata = readdata.substr(0, readdata.size() - 3);
	      oss << readdata;
	    }
	  }
	} while(chunk && !_gzip);
      } else
	oss << readdata;
      /* test support of gzip content */
      _plain = oss.str();
      if(_hdr.equals("Content-Encoding", "gzip")) {
	string t = _plain;
	_plain = inflate(t);
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
