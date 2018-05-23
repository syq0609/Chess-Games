#include "hall.h"

namespace hall{

void Hall::loadConfig()
{
    ProtoManager::me().loadConfig(m_cfgDir);
}

}
