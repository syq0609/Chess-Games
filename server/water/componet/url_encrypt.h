#ifndef WATER_COMPONET_URL_ENCRYPT_HPP
#define WATER_COMPONET_URL_ENCRYPT_HPP

#include <string>
#include <assert.h>

namespace water{
namespace componet{

    unsigned char toHex(unsigned char x);

    unsigned char fromHex(unsigned char x);

    std::string urlEncode(const std::string& str);

    std::string urlDecode(const std::string& str);

}}

#endif 


