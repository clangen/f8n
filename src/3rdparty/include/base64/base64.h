#ifndef __C_BASE_64_ENC_DEC_H__
#define __C_BASE_64_ENC_DEC_H__

#include <string>

std::string base64_encode(const std::string& string_to_encode);
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif