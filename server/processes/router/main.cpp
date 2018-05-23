/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-19 15:53 +0800
 *
 * Description: 
 */


#include "router.h"
#include "base/shell_arg_parser.h"

#include <csignal>
#include <iostream>

int main(int argc, char* argv[])
{
    water::process::ShellArgParser arg(argc, argv);

    router::Router::init(arg.num(), arg.configDir(), arg.logDir());

    router::Router::me().start();
}

