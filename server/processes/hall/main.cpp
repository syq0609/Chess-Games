/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 10:46 +0800
 *
 * Description:  场景逻辑处理服
 */

#include "hall.h"
#include "base/shell_arg_parser.h"

#include <csignal>

int main(int argc, char* argv[])
{
    using namespace hall;

    water::process::ShellArgParser arg(argc, argv);

    hall::Hall::init(arg.num(), arg.configDir(), arg.logDir());
    hall::Hall::me().start();
}
