#include "client.h"
#include "auto.h"

int main()
{
    AutoActions aact;
    aact.init();
    Client::me().run();
}
