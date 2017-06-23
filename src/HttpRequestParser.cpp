#include "HttpRequestParser.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

uint64_t HttpRequestParser::hash(const std::string& path)
{
  const std::string fake_req{
    std::string{"GET "} + path + std::string{" HTTP/1.1\r\n\r\n"}
  };
  HttpRequestParser parser;
  parser.read(fake_req.data(), fake_req.size());
  return parser.hashv();
}

HttpRequestParser::HttpRequestParser()
: state{0}
, hashs{0, 0}
{
}

const Response& HttpRequestParser::read(const char* buffer, uint size)
{
  // Minimal request size is 16: "GET  HTTP/1.1\r\n\r\n"
  if(size < 16) {
    // Received partial request (too small)
    return Response::bad_request;
  }

  // Check that it starts with  `GET ` (`0x47455420`)
  const uint32_t start = *reinterpret_cast<const uint32_t*>(buffer);
  if(start != 0x20544547) {
    // Only GET method is supported
    return Response::method_not_allowed;
  }

  // Find the second occurance of ` ` (`0x20`)
  const uint32_t end_of_request = hash_till_space(buffer + 4, size - 4 - 12);
  if(end_of_request >= size) {
    // End of URI not in sight.
    return Response::uri_to_long;
  }
  // const std::string req{buffer + 4, end_of_request};
  // std::cerr << "REQ [" << req << "]\n";
  // std::cerr << "Hash 0x" << std::hex << hashs[0] << hashs[1] << "\n";

  // Check for "HTTP/1.1\r\n" (`0x485454502F312E310D0A`)
  const uint64_t http1 = *reinterpret_cast<const uint64_t*>(buffer + 4 + end_of_request + 1);
  const uint16_t http2 = *reinterpret_cast<const uint16_t*>(buffer + 4 + end_of_request + 1 + 8);
  if(http1 != 0x312E312F50545448 || http2 != 0x0A0D) {
    // TODO: Allow HTTP/1.0 and disable keep-alive
    return Response::http_version_not_supported;
  }

  // TODO: Look for `\r\nConnection: close\r\n`?
  // TODO: Look for `\r\nIf-None-Match:`
  //       How do we deal with optional whitespace (space or tab)
  //       before and after the value?

  // If the end is '\r\n\r\n' (`0x0D0A0D0A`) we have the whole request.
  const uint32_t end = *reinterpret_cast<const uint32_t*>(buffer + size - 4);
  if(end != 0x0A0D0A0D) {
    // Received partial request (incomplete)
    return Response::request_to_large;
  }

  // Get the response
  return Response::get(hashs[0]);
}

#include <smmintrin.h> // SSE4.1
// SSE4.1 implementation of MurmurHash3_x86_128
// See: https://github.com/PeterScott/murmur3/blob/master/murmur3.c#L128
// NOTE There are some deviations!

// TODO: Is this really worth it?

inline __m128i rotl32(__m128i h, int r)
{
  return _mm_or_si128(_mm_srli_epi32(h, r),_mm_slli_epi32(h, 32 - r));
}

inline __m128i update(__m128i h, __m128i k)
{
  uint32_t c1 = 0x239b961b;
  uint32_t c2 = 0xab0e9789;
  uint32_t c3 = 0x38b34ae5;
  uint32_t c4 = 0xa1e38b93;

  // k1 *= c1; k2 *= c2; k3 *= c3; k4 *= c4;
  k = _mm_mullo_epi32(k, _mm_set_epi32(c4, c3, c2, c1));

  // k1  = ROTL32(k1,15); k2  = ROTL32(k2,16);
  // k3  = ROTL32(k3,17); k4  = ROTL32(k4,18);
  // NOTE We can not do separate rotates, we do 17 everywhere instead
  k = rotl32(k, 17);

  // k1 *= c2; k2 *= c3; k3 *= c4; k4 *= c1;
  k = _mm_mullo_epi32(k, _mm_set_epi32(c1, c4, c3, c2));

  // h1 ^= k1; h2 ^= k2; h3 ^= k3; h4 ^= k4;
  h = _mm_xor_si128(h, k);

  // h1 = ROTL32(h1,19); h2 = ROTL32(h2,17);
  // h3 = ROTL32(h3,15); h4 = ROTL32(h4,13);
  // NOTE We can not do separate rotates, we do 15 everywhere instead
  k = rotl32(k, 15);

  // h1 += h2; h2 += h3; h3 += h4; h4 += h1;
  // NOTE The value added to h4 is h1 *before* the other adds
  h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0x39));

  // h1 = h1*5+0x561ccd1b; h2 = h2*5+0x0bcaa747;
  // h3 = h3*5+0x96cd1c35; h4 = h4*5+0x32ac3b17;
  h = _mm_mullo_epi32(h, _mm_set1_epi32(5));
  h = _mm_add_epi32(h, _mm_set_epi32(0x32ac3b17, 0x96cd1c35, 0x0bcaa747, 0x561ccd1b));

  return h;
}

inline __m128i finalize(__m128i h, uint len)
{
  // h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
  h = _mm_xor_si128(h, _mm_set1_epi32(len));

  // h1 += h2; h1 += h3; h1 += h4;
  // h2 += h1; h3 += h1; h4 += h1;
  const __m128i vsuma1 = _mm_add_epi32(h, _mm_srli_si128(h, 8));
  const __m128i vsuma2 = _mm_add_epi32(vsuma1, _mm_srli_si128(vsuma1, 4));
  const uint32_t suma =  _mm_cvtsi128_si32(vsuma2);
  h = _mm_add_epi32(h, _mm_set_epi32(suma, suma, suma, 0));

  // h1 = fmix32(h1); h2 = fmix32(h2);
  // h3 = fmix32(h3); h4 = fmix32(h4);
  h = _mm_xor_si128(h, _mm_srli_epi32(h, 16));
  h = _mm_mullo_epi32(h, _mm_set1_epi32(0x85ebca6b));
  h = _mm_xor_si128(h, _mm_srli_epi32(h, 13));
  h = _mm_mullo_epi32(h, _mm_set1_epi32(0xc2b2ae35));
  h = _mm_mullo_epi32(h, _mm_srli_epi32(h, 16));

  // h1 += h2; h1 += h3; h1 += h4;
  // h2 += h1; h3 += h1; h4 += h1;
  const __m128i vsumb1 = _mm_add_epi32(h, _mm_srli_si128(h, 8));
  const __m128i vsumb2 = _mm_add_epi32(vsumb1, _mm_srli_si128(vsumb1, 4));
  const uint32_t sumb =  _mm_cvtsi128_si32(vsumb2);
  h = _mm_add_epi32(h, _mm_set_epi32(sumb, sumb, sumb, 0));

  return h;
}

inline uint HttpRequestParser::hash_till_space(const char *buffer, uint size)
{
  __m128i h = _mm_set1_epi32(0);
  const __m128i pattern = _mm_set1_epi8(0x20);

  // TODO: ERR This requires reading-past end! (up to 15 bytes)
  for(uint i = 0; i < size; i += 16) {

    // Check for spaces (using SSE2)
    const __m128i block   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + i));
    const __m128i matches = _mm_cmpeq_epi8(block, pattern);
    const uint32_t mask32 = _mm_movemask_epi8(matches);

    if(mask32 != 0) {

      // Add final block, zero out bytes beyond end
      __m128i mask = matches;
      mask = _mm_or_si128(mask, _mm_srli_si128(mask, 8));
      mask = _mm_or_si128(mask, _mm_srli_si128(mask, 4));
      mask = _mm_or_si128(mask, _mm_srli_si128(mask, 2));
      mask = _mm_or_si128(mask, _mm_srli_si128(mask, 1));
      const __m128i final_block = _mm_andnot_si128(block, mask);
      h = update(h, final_block);

      // Compute length
      const uint32_t offset = __builtin_ctz(mask32);
      const uint32_t len = i + offset;

      // Finalize and store hash
      h = finalize(h, len);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(&(this->hashs)), h);

      return len;
    }

    // Update
    h = update(h, block);
  }
  // TODO: return some error
  throw std::runtime_error{
    "Invalid request"
  };
}
