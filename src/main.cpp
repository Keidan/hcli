/**
 *******************************************************************************
 * <p><b>Project hcli</b><br/>
 * </p>
 * @author Keidan
 *
 *******************************************************************************
 */
#include <iostream>
#include <cstdio>
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

constexpr const char* APPNAME = "hcli";
static HttpClient client(APPNAME);
static std::ifstream is_params;

static const struct option long_options[] = { 
    { "help"      , 0, NULL, 'h' },
    { "host"      , 1, NULL, '0' },
    { "ssl"       , 0, NULL, 's' },
    { "method"    , 1, NULL, 'm' },
    { "header"    , 1, NULL, '1' },
    { "cookie"    , 1, NULL, '2' },
    { "param"     , 1, NULL, '3' },
    { "gzip"      , 0, NULL, 'g' },
    { "params"    , 1, NULL, '4' },
    { "headers"   , 1, NULL, '5' },
    { "urlencode" , 0, NULL, '6' },
    { "uexcept"   , 1, NULL, '7' },
    { "form"      , 0, NULL, '8' },
    { NULL        , 0, NULL,  0  } 
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
  cout << "usage: hclient options" << endl;
  cout << "Possible host values: " << endl;
  cout << "--host www.ralala.fr (use port 80 and page /)" << endl;
  cout << "--host www.ralala.fr -s (use port 443 and page /)" << endl;
  cout << "--host www.ralala.fr:8443 -s (use port 8443 and page /)" << endl;
  cout << "--host www.ralala.fr:8080/login (use port 8080 and page /login)" << endl;
  cout << "\t--help, -h: Print this help." << endl;
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
  exit(err);
}

void print_hex(FILE* std, unsigned char* buffer, int len, bool print_raw) {
  int i = 0, max = 16, loop = len;
  unsigned char *p = buffer;
  char line [max + 3]; /* spaces + \0 */
  memset(line, 0, sizeof(line));
  while(loop--) {
    unsigned char c = *(p++);
    if(!print_raw) {
      fprintf(std, "%02x ", c);
      /* only the visibles char */
      if(c >= 0x20 && c <= 0x7e) line[i] = c;
      /* else mask with '.' */
      else line[i] = 0x2e; /* . */
    } else fprintf(std, "%02x", c);
    /* next line */
    if(i == max) {
      if(!print_raw)
	fprintf(std, "  %s\n", line);
      else fprintf(std, "\n");
      /* re init */
      i = 0;
      memset(line, 0, sizeof(line));
    }
    /* next */
    else i++;
    /* add a space in the midline */
    if(i == max / 2 && !print_raw) {
      fprintf(std, " ");
      line[i++] = 0x20;
    }
  }
  /* align 'line'*/
  if(i != 0 && (i < max || i <= len) && !print_raw) {
    while(i++ <= max) fprintf(std, "   "); /* 3 spaces ex: "00 " */
    fprintf(std, "%s\n", line);
  }
  fprintf(std, "\n");
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
  cnx.method = "GET";
  cnx.host = cnx.uexcept = "";
  cnx.gzip = cnx.ssl = cnx.urlencode = cnx.isform = false;
  cnx.is_params = &is_params;

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = &signal_hook;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  atexit(shutdown_hook);


  int opt;
  while ((opt = getopt_long(argc, argv, "h0:sm:1:2:3:g4:5:67:8", long_options, NULL)) != -1) {
    switch (opt) {
      case 'h': usage(0); break;
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
	size_t pos = content.find("\n");
	while(pos != string::npos) {
	  string line = content.substr(0, pos);
	  content = content.substr(pos+1);
	  if(line.at(0) == '#') continue;
	  size_t ppos = line.find("=");
	  if(ppos != string::npos)
	    cnx.headers[line.substr(0, ppos)] = line.substr(ppos+1);
	  pos = content.find("\n");
	}
	break;
      }
      case '6': cnx.urlencode = true; break;
      case '7': cnx.uexcept = string(optarg); break;
      case '8': cnx.isform = true; break;
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
    //print_hex(stdout, (unsigned char*)plain.c_str(), plain.length(), true);

    /* print the respponse informations*/
    cout << "Response code " << hdr.code() << ", reason: '" << hdr.reason() << "'" << endl;
    cout << "List of headers (length: " << hdr.length() << "):" << endl;
    helper::vstring hkeys = hdr.keys();
    for(helper::vstring::const_iterator it = hkeys.begin(); it != hkeys.end(); ++it) {
      string key = (*it);
      helper::vstring values = hdr.get(key);
      for(helper::vstring::const_iterator jt = values.begin(); jt != values.end(); ++jt) {
	cout << " - " << key << ": " << (*jt) << endl;
      }
    }
    cout << "Body length:" << plain.length() << endl;
    if(!plain.empty())
      cout << "=====" << plain << "=====" << endl;
    //print_hex(stdout, (unsigned char*)plain.c_str(), plain.length(), true);
    //cout << endl << endl;
  } catch (std::exception& e)  {
    cerr << e.what() << endl;
  }
  return 0;
}
