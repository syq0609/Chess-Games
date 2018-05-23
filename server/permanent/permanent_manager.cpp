#include "permanent_manager.h"

#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"

namespace permanent {

ProtoMsgPtr create(const std::string& pbName)
{
    const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(pbName);
    if (descriptor)
    {
        const ProtoMsg* prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            ProtoMsgPtr msg(prototype->New());
            return msg;
        }
    }
    return nullptr;
}


}
