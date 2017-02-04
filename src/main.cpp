/**
 *******************************************************************************
 * <p><b>Project httpu</b><br/>
 * </p>
 * @author Keidan
 *
 *******************************************************************************
 */
#include <iostream>
#include <signal.h>
#include <cstring>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <streambuf>

#include <sys/types.h>
#include "HttpClient.hpp" 
#include "Helper.hpp" 


using std::cerr;
using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using std::size_t;
using std::ifstream;
using net::http::HttpClient;
using net::http::HttpClientConnect;
using net::http::HttpHeader;
using helper::Helper;
using helper::vstring;

constexpr const char* APPNAME = "hcli";
static HttpClient client(APPNAME);
static std::ifstream is_params;

static const struct option long_options[] = { 
    { "help"        , 0, NULL, 'h' },
    { "verbose"     , 1, NULL, 'v' },
    { "host"        , 1, NULL, '0' },
    { "ssl"         , 0, NULL, 's' },
    { "method"      , 1, NULL, 'm' },
    { "header"      , 1, NULL, '1' },
    { "cookie"      , 1, NULL, '2' },
    { "param"       , 1, NULL, '3' },
    { "gzip"        , 0, NULL, 'g' },
    { "params"      , 1, NULL, '4' },
    { "headers"     , 1, NULL, '5' },
    { "urlencode"   , 0, NULL, '6' },
    { "uexcept"     , 1, NULL, '7' },
    { "form"        , 0, NULL, '8' },
    { "multipart"   , 1, NULL, '9' },
    { "multiparts"  , 1, NULL, 'A' },
    { NULL          , 0, NULL,  0  } 
};

auto signal_hook(int s) -> void { 
  cout << "Signal: " << s << endl;
  exit(s);
}

auto shutdown_hook() -> void {
  if(client.ssl()) net::EasySocket::unloadSSL();
  if(is_params.is_open()) is_params.close();
  cout << "Bye bye." << endl;
}

auto usage(int err) -> void {
  cout << "usage: httpu options" << endl;
  cout << "Possible host values: " << endl;
  cout << "--host www.ralala.fr (use port 80 and page /)" << endl;
  cout << "--host www.ralala.fr -s (use port 443 and page /)" << endl;
  cout << "--host www.ralala.fr:8443 -s (use port 8443 and page /)" << endl;
  cout << "--host www.ralala.fr:8080/login (use port 8080 and page /login)" << endl;
  cout << "\t--help, -h: Print this help." << endl;
  cout << "\t--verbose -v: Verbose mode, possible values all|query|chunk|rhex|rhdr|rraw (use '|' to combine)." << endl;
  cout << "\t\tall: Full verbose mode." << endl;
  cout << "\t\tquery: Prints the query used to contact the remote host." << endl;
  cout << "\t\tchunk: Prints the chunk information (if available) of the server response." << endl;
  cout << "\t\trhdr: Prints the response headers (and cookies)." << endl;
  cout << "\t\trraw: Prints the whole response." << endl;
  cout << "\t\trhex: Prints the response in hex format." << endl;
  cout << "\t--host: Host address." << endl;
  cout << "\t--ssl, -s: Use SSL." << endl;
  cout << "\t--gzip, -g: Use Accept-Encoding: gzip." << endl;
  cout << "\t--method, -m: HTTP method.." << endl;
  cout << "\t--header: Add new header to the query (format key=value)." << endl;
  cout << "\t--headers: A file with the headers." << endl;
  cout << "\t--cookie: Add new cookie to the query (format value)." << endl;
  cout << "\t--param: Add new param to the query (format key=value)." << endl;
  cout << "\t--params: A file with the content to use as body request (unavailable with GET method)." << endl;
  cout << "\t--form: A file with the content to use as body request (unavailable with GET method)." << endl;
  cout << "\t--urlencode: URL encode the params." << endl;
  cout << "\t--uexcept: The list of characters that are not encoded in URL format." << endl;
  cout << "\t--multipart: Add new header to the query multipart(format key=value)." << endl;
  cout << "\t--multiparts: A file with the headers." << endl;
  exit(err);
}


auto readFile(const char* filename) -> string {
  ifstream ifs(filename);
  if(!ifs.is_open()) {
    cerr << "Unable to open the file " << filename << endl;
    exit(1);
  }
  string content;
  ifs.seekg(0, std::ios::end);   
  content.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);
  content.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
  ifs.close();
  return content;
}

int  main(int argc, char** argv) {
  HttpClientConnect cnx;
  struct sigaction sa;
  bool print_hdr = false;
  cnx.method = "GET";
  cnx.host = cnx.uexcept = "";
  cnx.gzip = cnx.ssl = cnx.urlencode = cnx.isform = cnx.print_query = cnx.print_hex = cnx.print_chunk = false;
  cnx.is_params = &is_params;
  cnx.print_nothing = false;

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = &signal_hook;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  atexit(shutdown_hook);


  int opt;
  while ((opt = getopt_long(argc, argv, "hv:0:sm:1:2:3:g4:5:67:89:A:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'h': usage(0); break;
      case 'v': {
	string verbose = string(optarg);
	vstring vs = Helper::split(string(optarg), '|');
	for(vstring::iterator it = vs.begin(); it != vs.end(); ++it) {
	  string v = *it;
	  std::transform(v.begin(), v.end(), v.begin(), ::toupper); 
	  if(v == "ALL")
	    print_hdr = cnx.print_query = cnx.print_chunk = true;
	  else if(v == "RHDR")
	    print_hdr = true;
	  else if(v == "CHUNK")
	    cnx.print_chunk = true;
	  else if(v == "QUERY")
	    cnx.print_query = true;
	  else if(v == "RRAW")
	    cnx.print_raw_resp = true;
	  else if(v == "RHEX")
	    cnx.print_hex = true;
	  else if(v == "NOTHING")
	    cnx.print_nothing = true;
	  else {
	    cerr << "Invalid verbose opton: " << optarg << endl;
	    usage(1);
	  }
	}
	break;
      }
      case '0': cnx.host = string(optarg); break;
      case 's': cnx.ssl = true; break;
      case 'g': cnx.gzip = true; break;
      case 'm':
	cnx.method = string(optarg);
	std::transform(cnx.method.begin(), cnx.method.end(), cnx.method.begin(), ::toupper); 
	break;
      case '1': { /* headers */
	string s(optarg);
	size_t found = s.find("=");
	if(found == string::npos) {
	  cerr << "Invalid header format (key=value): " << optarg << endl;
	  exit(1);
	}
	cnx.headers[s.substr(0, found)] = s.substr(found + 1);
	break;
      }
      case '2': cnx.cookies.push_back(string(optarg)); break;
      case '3': { /* params */
	string s(optarg);
	size_t found = s.find("=");
	if(found == string::npos) {
	  cerr << "Invalid param format (key=value): " << optarg << endl;
	  exit(1);
	}
	cnx.params[s.substr(0, found)] = s.substr(found + 1);
	break;
      }
      case '4': /* is_params */
	is_params.open(optarg, std::ios::binary | std::ios::ate);
	if(!is_params.is_open()) {
	  cerr << "Unable to open the file " << optarg << endl;
	  exit(1);
	}
	break;
      case '5': { /* headers file */
	string content = readFile(optarg);
	vstring vs = Helper::split(content, '\n');
	for(vstring::iterator it = vs.begin(); it != vs.end(); ++it) {
	  string line = *it;
	  if(line.at(0) == '#') continue;
	  vstring _vs = Helper::split(line, '=');
	  if(_vs.size() == 2)
	    cnx.headers[_vs[0]] = _vs[1];
	  else if(_vs.size() > 2)
	    cnx.headers[_vs[0]] = line.substr(_vs[0].length() + 1);
	}
	break;
      }
      case '6': cnx.urlencode = true; break;
      case '7': cnx.uexcept = string(optarg); break;
      case '8': cnx.isform = true; break;
      case '9': { /* multipart */
	string s(optarg);
	size_t found = s.find("=");
	if(found == string::npos) {
	  cerr << "Invalid header format (key=value): " << optarg << endl;
	  exit(1);
	}
	cnx.multiparts[s.substr(0, found)] = s.substr(found + 1);
	break;
      }
      case 'A': { /* multiparts file */
	string content = readFile(optarg);
	vstring vs = Helper::split(content, '\n');
	for(vstring::iterator it = vs.begin(); it != vs.end(); ++it) {
	  string line = *it;
	  if(line.at(0) == '#') continue;
	  vstring _vs = Helper::split(line, '=');
	  if(_vs.size() == 2)
	    cnx.multiparts[_vs[0]] = _vs[1];
	  else if(_vs.size() > 2)
	    cnx.multiparts[_vs[0]] = line.substr(_vs[0].length() + 1);
	}
	break;
      }
      default: cerr << "Unknown option" << endl; usage(-1); break;
    }
  }
  if(cnx.ssl && !net::EasySocket::loadSSL()) {
    cerr << "Unable to load SSL : " << net::EasySocket::lastErrorSSL() << endl;
    exit(1);
  }
  if(cnx.method == "GET" && cnx.is_params->is_open()) {
    cerr << "Unable to use the parameters file with GET method" << endl;
    exit(1);
  }

  
  try {
    client.connect(cnx);
   

    HttpHeader& hdr = client.getHttpHeader();
 
    /* test support of gzip content */
    string& plain = client.getPlainText();

    /* print the respponse informations*/
    cout << "Response code " << hdr.code() << ", reason: '" << hdr.reason() << "'" << endl;
    if(print_hdr) {
      cout << "List of headers (length: " << Helper::toHumanStringSize(hdr.length()) << "):" << endl;
      helper::vstring hkeys = hdr.keys();
      for(helper::vstring::const_iterator it = hkeys.begin(); it != hkeys.end(); ++it) {
	string key = (*it);
	helper::vstring values = hdr.get(key);
	for(helper::vstring::const_iterator jt = values.begin(); jt != values.end(); ++jt) {
	  cout << " - " << key << ": " << (*jt) << endl;
	}
      }
    }
    cout << "Body length:" << Helper::toHumanStringSize(plain.size()) << endl;
    if(!plain.empty())
      cout << "=====" << plain << "=====" << endl;
   
    //cout << endl << endl;
  } catch (std::exception& e)  {
    cerr << e.what() << endl;
  }
  return 0;
}
