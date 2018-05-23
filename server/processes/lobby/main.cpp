/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 11:25 +0800
 *
 * Description: lobby 启动入口
 *              本进程为大堂的抽象，在这里进行房间分配，游戏项目选择，队友匹配聊天等
 */

#include "lobby.h"
#include "base/shell_arg_parser.h"

#include <csignal>
#include <iostream>

int main(int argc, char* argv[])
{
    water::process::ShellArgParser arg(argc, argv);

    lobby::Lobby::init(arg.num(), arg.configDir(), arg.logDir());

    lobby::Lobby::me().start();
}

