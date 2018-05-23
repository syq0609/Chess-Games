/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-27 16:06 +0800
 *
 * Description:  引用nlohmann/json库, 仅修改使文件名和现有文件名风格统一
 *               来源:	https://github.com/nlohmann/json
 *               git:   git@github.com:nlohmann/json.git
 */

# ifndef WATER_JSON_H

#include "nlohmann/json.hpp"

namespace water{
namespace componet{

using json = nlohmann::json;

}}

# define WATER_JSON_H
# endif
