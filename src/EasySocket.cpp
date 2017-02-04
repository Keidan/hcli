/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#include "EasySocket.hpp" 
#include <sstream>  
#include <openssl/err.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#define throw_libc(m) do {						\
    std::ostringstream oss;						\
    oss << "[[" << __LINE__ << "]] ";					\
    oss << strerror(errno);						\
    throw EasySocketException(oss.str());				\
  } while(0)

#define throw_ssl0(m, i) do {				\
    std::ostringstream oss;						\
    oss << "[[" << __LINE__ << "]] ";					\
    _lib_ssl_errno = i;							\
    oss << lastErrorSSL();						\
    throw EasySocketException(oss.str());				\
  } while(0)
#define throw_ssl(m) throw_ssl0(m, ERR_get_error())

/**
 * @fn static int easy_socket_dumb_callback(int preverify_ok, X509_STORE_CTX *ctx)
 * @brief Dumb callback used by the SSL_CTX_set_verify function.
 * @param preverify_ok preverify ok.
 * @param ctx X509 store context.
 * @return 1 to continue.
 */
static int easy_socket_dumb_callback(int preverify_ok, X509_STORE_CTX *ctx) {
  (void)preverify_ok;
  (void)ctx;
  return 1;
}

namespace net {
  unsigned long EasySocket::_lib_ssl_errno = 0L;
  constexpr unsigned short PORT_HTTP       = 80;
  constexpr unsigned short PORT_HTTPS      = 443;

  /**
   * @brief Initialize the SSL stack, should be called only once in your application.
   * @return false if the initialization fail, true else.
   */
  auto EasySocket::loadSSL() -> bool {
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    if(SSL_library_init() < 0)
      return false;
    return true;
  }

  /**
   * @brief Release the SSL stack, should be called only once in your application.
   */
  auto EasySocket::unloadSSL() -> void {
    ERR_free_strings();
    EVP_cleanup();
  }

  /**
   * Get the last SSL error
   */
  auto EasySocket::lastErrorSSL() -> std::string {
    return std::string(ERR_error_string(_lib_ssl_errno, NULL));
  }

  EasySocket::EasySocket() : _fd(-1), _useSSL(false), _open(false), _ctx(nullptr), _ssl(nullptr) {
  }

  EasySocket::~EasySocket() {
    disconnect();
  }

  /**
   * @brief Close the socket with the remote address.
   */
  auto EasySocket::disconnect() -> void {
    if(_ctx != nullptr) {
      SSL_CTX_free(_ctx);   
      _ctx = nullptr;
    }
    if(_ssl != nullptr) {
      SSL_free(_ssl); 
      _ssl = nullptr;
    }
    if(_fd != -1) close(_fd);
    _open = false;
  }
  /**
   * @brief Connect the socket to the remote address.
   * @param host The remote address.
   * @param port The remote port.
   * @return EasySocketResult
   */
  auto EasySocket::connect(std::string host, int port) -> void { 
    _open = false;

    if ((_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
      throw_libc("Cannot create socket: ");

    struct hostent *remoteh;
    /* Look up the remote host to get its network number. */
    if ((remoteh = ::gethostbyname(host.c_str())) == NULL) {
      disconnect();
      throw_libc("Cannot resolv host: ");
    }

    struct sockaddr_in address;
    /* Initialize the address varaible, which specifies where connect() should attempt to connect. */
    bcopy(remoteh->h_addr, &address.sin_addr, remoteh->h_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (!(::connect(_fd, (struct sockaddr *)(&address), sizeof(address)) >= 0)) {
      disconnect();
      throw_libc("Cannot connect: ");
    }
    if(!_useSSL) {
      _open = true;
      return;
    }

    /* We first need to establish what sort of */
    /* connection we know how to make. We can use one of */
    /* SSLv23_client_method(), SSLv2_client_method() and */
    /* SSLv3_client_method(). */
    /*  Try to create a new SSL context. */
    if(( _ctx = SSL_CTX_new(SSLv23_client_method())) == nullptr) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      throw_ssl("SSL Error: ");
    }
    SSL_CTX_set_mode(_ctx, SSL_MODE_AUTO_RETRY);

    /* Set it up so tha we will connect to *any* site, regardless of their certificate. */
    SSL_CTX_set_verify(_ctx, SSL_VERIFY_NONE, easy_socket_dumb_callback);
    /* Enable bug support hacks. */
    SSL_CTX_set_options(_ctx, SSL_OP_ALL);


    if ((_ssl = SSL_new(_ctx)) == NULL) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      throw_ssl("SSL Error: ");
    }
    SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY); 

    if (SSL_set_fd(_ssl, _fd) == 0) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      throw_ssl("SSL Error: ");
    }
    if (SSL_connect(_ssl) == -1) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      throw_ssl("SSL Error: ");
    }
    

    _open = true;
  }


  /**
   * @brief Write some data to the socket descriptor.
   * @param toWrite The data to write.
   */
  auto EasySocket::write(const std::string toWrite) -> void {
    if(_useSSL) {
      for (;;) {
	int rc1 = SSL_write(_ssl, toWrite.c_str(), toWrite.size());
	int rc2 = SSL_get_error(_ssl, rc1);
	switch (rc2) {
	  case SSL_ERROR_NONE:
	  case SSL_ERROR_ZERO_RETURN:
	    return;
	  case SSL_ERROR_WANT_READ:
	  case SSL_ERROR_WANT_WRITE: {
	    throw_ssl("Want write error (" + std::to_string(rc1) + "): ");
	    break;
	  }
	  default:
	    throw_ssl("SSL Write error (" + std::to_string(rc1) + "): ");
	}
      }
    }
    else {
      int w = ::write(_fd, toWrite.c_str(), toWrite.size());
      if(w < 0)
	throw_libc("Write error (" + std::to_string(w) + "): ");
    }
  }

  /**
   * @brief Read some data from the socket descriptor.
   * @param toRead The data reads.
   */
  auto EasySocket::read(std::string &toRead) -> void {
    char buffer[1024];
    std::ostringstream oss;
    bool leave = false;
    if(_useSSL) {
      for(;;) {
	bzero(buffer, 1024);
	int rc1 = SSL_read(_ssl, buffer, 1023);
	int rc2 = SSL_get_error(_ssl, rc1);
	if(rc1 > 0) oss << buffer;
	switch (rc2) {
	  case SSL_ERROR_SYSCALL:
	    if(rc1) {
	      throw_ssl("Read read error (" + std::to_string(rc2) + "): ");
	    }
	  case SSL_ERROR_NONE:
	  case SSL_ERROR_ZERO_RETURN:
	    leave = true;
	    break;
	  case SSL_ERROR_WANT_READ:
	  case SSL_ERROR_WANT_WRITE: {
	    throw_ssl("Want read error (" + std::to_string(rc2) + "): ");
	    break;
	  }
	  default: {
	    throw_ssl("SSL read error (" + std::to_string(rc2) + "): ");
	    break;
	  }
	}
	if(leave) break;
      }
    } else {
      int reads = ::read(_fd, buffer, 1023);
      if(reads < 0) {
	throw_libc("Read error (" + std::to_string(reads) + "): ");
      }
      buffer[reads] = 0;
      oss << buffer;
    }
    toRead = oss.str();
  }

  /**
   * @brief Change the SSL status.
   * @param useSSL SSL status.
   */
  auto EasySocket::ssl(int useSSL) -> void {
    if(!_open) _useSSL = useSSL;
  }

  /**
   * @brief Get The SSL status.
   * @return bool
   */
  auto EasySocket::ssl() -> bool {
    return _useSSL;
  }

  /**
   * @brief Get the socket descriptor.
   * @return int
   */
  auto EasySocket::fd() -> int {
    return _fd;
  }

} /* namespace net */
