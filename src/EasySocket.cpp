/**
*******************************************************************************
* <p><b>Project hcli</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#include "EasySocket.hpp" 
#include <openssl/evp.h> 
#include <openssl/err.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


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

  constexpr unsigned int BUF_SZ = 1024;
  unsigned long EasySocket::_lib_ssl_errno = 0L;

  /**
   * @brief Initialize the SSL stack, should be called only once in your application.
   * @return false if the initialization fail, true else.
   */
  auto EasySocket::loadSSL() -> bool {
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    _lib_ssl_errno = ERR_get_error();
    if(SSL_library_init() < 0) {
      _lib_ssl_errno = ERR_get_error();
      return false;
    }
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

  EasySocket::EasySocket() : _fd(-1), _ssl(false), _rEnd(false), _open(false), _lib_ssl_ctx(nullptr), _lib_ssl(nullptr) {
  }

  EasySocket::~EasySocket() {
    disconnect();
  }

  /**
   * @brief Close the socket with the remote address.
   */
  auto EasySocket::disconnect() -> void {
    if(_lib_ssl != nullptr) {
      SSL_shutdown(_lib_ssl);
      SSL_free(_lib_ssl);
      _lib_ssl = nullptr;
    }
    if(_lib_ssl_ctx != nullptr) {
      SSL_CTX_free(_lib_ssl_ctx);
      _lib_ssl_ctx = nullptr;
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
  auto EasySocket::connect(std::string host, int port) -> EasySocketResult { 
    _open = false;
    if ((_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return EasySocketResult::ERROR_SOCKET;

    struct hostent *remoteh;
    /* Look up the remote host to get its network number. */
    if ((remoteh = ::gethostbyname(host.c_str())) == NULL) {
      disconnect();
      return EasySocketResult::ERROR_RESOLV_HOST;
    }

    struct sockaddr_in address;
    /* Initialize the address varaible, which specifies where connect() should attempt to connect. */
    bcopy(remoteh->h_addr, &address.sin_addr, remoteh->h_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (!(::connect(_fd, (struct sockaddr *)(&address), sizeof(address)) >= 0)) {
      disconnect();
      return EasySocketResult::ERROR_CONNECT;
    }

    if(!_ssl) {
      _open = true;
      return EasySocketResult::OK;
    }
    /* We first need to establish what sort of */
    /* connection we know how to make. We can use one of */
    /* SSLv23_client_method(), SSLv2_client_method() and */
    /* SSLv3_client_method(). */
    /*  Try to create a new SSL context. */
    if(( _lib_ssl_ctx = SSL_CTX_new(SSLv23_client_method())) == nullptr) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      return EasySocketResult::ERROR_SSL_CTX_NEW;
    }

    /* Set it up so tha we will connect to *any* site, regardless of their certificate. */
    SSL_CTX_set_verify(_lib_ssl_ctx, SSL_VERIFY_NONE, easy_socket_dumb_callback);
    /* Enable bug support hacks. */
    SSL_CTX_set_options(_lib_ssl_ctx, SSL_OP_ALL);

    /* Create new SSL connection state object. */
    _lib_ssl = SSL_new(_lib_ssl_ctx);
    if(_lib_ssl == nullptr) {
      _lib_ssl_errno = ERR_get_error();
      disconnect();
      return EasySocketResult::ERROR_SSL_NEW;
    }
  
    /* Attach the SSL session. */
    SSL_set_fd(_lib_ssl, _fd);
    /* Connect using the SSL session. */
    if(SSL_connect(_lib_ssl) != 1) {
      _lib_ssl_errno = ERR_PACK(ERR_LIB_SYS, SYS_F_CONNECT, ERR_R_SYS_LIB);
      disconnect();
      return EasySocketResult::ERROR_SSL_CONNECT;
    }
    _open = true;
    return EasySocketResult::OK;
  }


  /**
   * @brief Write some data to the socket descriptor.
   * @param toWrite The data to write.
   */
  auto EasySocket::write(const std::string toWrite) -> void {
    int w;
    if(_ssl)
      w = SSL_write(_lib_ssl, toWrite.c_str(), toWrite.length());
    else
      w = ::write(_fd, toWrite.c_str(), toWrite.length());
    if(w < 0 || (_ssl && w == 0)) {
      std::string msg = "Write error (" + std::to_string(w) + "): ";
      if(_ssl) {
	_lib_ssl_errno = ERR_get_error();
	//msg += lastErrorSSL();
      } else
	msg += strerror(errno);
      throw EasySocketException(msg);
    }
  }

  /**
   * @brief Read some data from the socket descriptor.
   * @param toRead The data reads.
   */
  auto EasySocket::read(std::string &toRead) -> void {
    _rEnd = false;
    int r;
    char buffer[BUF_SZ];
    if(_ssl) {
      if((r = SSL_read(_lib_ssl, buffer, BUF_SZ)) <= 0)
	_rEnd = true;
    } else {
      if((r = ::read(_fd, buffer, BUF_SZ)) <= 0)
	_rEnd = true;
    }
    if(r < 0) {
      std::string msg = "Read error (" + std::to_string(r) + "): ";
      if(_ssl) {
	_lib_ssl_errno = ERR_get_error();
	msg += lastErrorSSL();
      } else
	msg += strerror(errno);
      throw EasySocketException(msg);
    } else {
      toRead = buffer;
      if(r < ((int)BUF_SZ)) _rEnd = true;
    }
  }

  /**
   * @brief Change the SSL status.
   * @param ssl SSL status.
   */
  auto EasySocket::ssl(int ssl) -> void {
    if(!_open) _ssl = ssl;
  }

  /**
   * @brief Get The SSL status.
   * @return bool
   */
  auto EasySocket::ssl() -> bool {
    return _ssl;
  }

  /**
   * @brief Get the socket descriptor.
   * @return int
   */
  auto EasySocket::fd() -> int {
    return _fd;
  }

  /**
   * @brief Test if EOF is reached.
   * @return bool
   */
  auto EasySocket::isEOF() -> bool {
    return _rEnd && _open;
  }

} /* namespace net */
