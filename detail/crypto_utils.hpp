#include <openssl/sha.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#pragma once


namespace merkle {

template <typename Span>
void computeSHA256(const Span& input, unsigned char* output) {
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, input.data(), input.size());
  SHA256_Final(output, &sha256);
}
};  // namespace merkle