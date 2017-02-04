/**
*******************************************************************************
* <p><b>Project httpu</b><br/>
* </p>
* @author Keidan
*
*******************************************************************************
*/
#ifndef __GZIP_H__
#define __GZIP_H__

#include <string>
#include <zlib.h>

namespace utils {

  enum class GZIPMethod : unsigned char {
      GZ,
      DEFLATE
  };

  class GZIP {
    public:
      GZIP() = default;
      ~GZIP() = default;


      /**
       * @brief Compress data.
       * @param str Plain data.
       * @param method Compression method.
       * @param compressionlevel Compression level.
       * @return Compressed data data.
       */
      static auto compress(const std::string &str, const GZIPMethod &method, int compressionlevel = Z_BEST_COMPRESSION) -> std::string;

     
      /**
       * @brief Decompress data.
       * @param str Compressed data.
       * @param method Decompression method.
       * @return Decompressed data data.
       */
     static auto decompress(const std::string &str, const GZIPMethod &method) -> std::string;
  };

} /* namespace utils */
#endif /* __GZIP_H__ */


