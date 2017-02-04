/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author https://blog.cppse.nl/deflate-and-gzip-compress-and-decompress-functions
*
*******************************************************************************
*/
#include "GZIP.hpp"
#include <cstring>
#include <stdexcept>
#include <sstream>

using std::stringstream;
using std::string;

namespace utils {

  constexpr unsigned char MOD_GZIP_ZLIB_WINDOWSIZE = 15;
  constexpr unsigned char MOD_GZIP_ZLIB_CFACTOR = 9;
  constexpr unsigned int  MOD_GZIP_ZLIB_BSIZE = 8096;

  /**
   * @brief Compress data.
   * @param str Plain data.
   * @param method Compression method.
   * @param compressionlevel Compression level.
   * @return Compressed data data.
   */
  auto GZIP::compress(const string &str, const GZIPMethod &method, int compressionlevel) -> string {
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));
    if(method == GZIPMethod::DEFLATE) {
      if (deflateInit(&zs, compressionlevel) != Z_OK)
	throw(std::runtime_error("deflateInit failed while compressing."));
    } else {
      if (deflateInit2(&zs, 
		       compressionlevel,
		       Z_DEFLATED,
		       MOD_GZIP_ZLIB_WINDOWSIZE + 16, 
		       MOD_GZIP_ZLIB_CFACTOR,
		       Z_DEFAULT_STRATEGY) != Z_OK
	 ) {
	throw(std::runtime_error("deflateInit2 failed while compressing."));
      }
    }
    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    string outstring;

    // retrieve the compressed bytes blockwise
    do {
      zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);

      ret = deflate(&zs, Z_FINISH);

      if (outstring.size() < zs.total_out) {
	// append the block to the output string
	outstring.append(outbuffer, zs.total_out - outstring.size());
      }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
      throw(std::runtime_error(oss.str()));
    }

    return outstring;
  }

  /**
   * @brief Decompress data.
   * @param str Compressed data.
   * @param method Decompression method.
   * @return Decompressed data data.
   */
  auto GZIP::decompress(const string &str, const GZIPMethod &method) -> string {
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if(method == GZIPMethod::DEFLATE) {
      if (inflateInit(&zs) != Z_OK)
	throw(std::runtime_error("inflateInit failed while decompressing."));
    } else {
      if (inflateInit2(&zs, MOD_GZIP_ZLIB_WINDOWSIZE + 16) != Z_OK)
	throw(std::runtime_error("inflateInit failed while decompressing."));
    }
    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    string outstring;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
      zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);

      ret = inflate(&zs, 0);

      if (outstring.size() < zs.total_out) {
	outstring.append(outbuffer,
			 zs.total_out - outstring.size());
      }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib deflate decompression: (" << ret << ") "
	  << zs.msg;
      throw(std::runtime_error(oss.str()));
    }

    return outstring;
  }


} /* namespace utils */
