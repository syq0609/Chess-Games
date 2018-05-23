/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-20 19:45 +0800
 *
 * Description: 
 */

#ifndef PERMANENT_MANAGER_H
#define PERMANENT_MANAGER_H

#include "google/protobuf/message.h"

namespace permanent
{

typedef google::protobuf::Message ProtoMsg;
typedef std::shared_ptr<ProtoMsg> ProtoMsgPtr;


ProtoMsgPtr create(const std::string& name);



}
#endif

