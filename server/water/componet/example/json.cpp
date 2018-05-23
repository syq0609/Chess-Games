#include "../json.h"

#include <iostream>
using namespace std;



int main()
{
    std::string jsonstr = R"({"status":"ok","data":{"openid":"oC4CE0a3zMyh2EjtFyguyG31KWos","access_token":"-bSGYKqb56NYVqJUzeJMOSC54unbwgf93efnWTMOwf5NnruqYQYK2itbcfHn-w74be2JHGTfS7S2yHH7E420pyTlUCw5hU0GfTTa2ZRUfO8","expires_in":7200,"refresh_token":"-bSGYKqb56NYVqJUzeJMOarACVEd6f7ZZtRMQ1RfLmmM3iMc_jr4-bdSFXm7bhgcvlBITQUvNwwbY_J7-DHJ-g5j2CXNidhlHVMqc83FCUY","scope":"snsapi_base,snsapi_userinfo,","user_info":{"openid":"oC4CE0a3zMyh2EjtFyguyG31KWos","nickname":"\u674e\u7532","sex":1,"language":"en","city":"Songjiang","province":"Shanghai","country":"CN","headimgurl":"http:\/\/wx.qlogo.cn\/mmopen\/csalNeIiav3rtzHicD3jlicBic5tzeQHl52iaJPW8bUCbN34mQtg6xl2ic2Xiack18aFVzFB184G1vGcicCU6OOvHIrYoz0lIicmyqPCl\/0","privilege":[],"unionid":"oqMdI1gFGE9LU-y4CrFfs5xr7DHY"}},"common":{"channel":"111267","user_sdk":"iOS_wxpay","uid":"oC4CE0a3zMyh2EjtFyguyG31KWos","server_id":"1","plugin_id":"399"},"ext":""})";

    cout <<  "raw: " << jsonstr << endl;

    using json = water::json;

    auto j = json::parse(jsonstr);
    cout << "openid:  " << j["data"]["openid"] << endl;
    cout << "expires: " << j["data"]["expires_in"] << endl;
    cout << "token::  " << j["data"]["access_token"] << endl;

    std::string openid   = j["data"]["openid"];
    uint32_t expires  = j["data"]["expires_in"];
    std::string token    = j["data"]["access_token"];

    cout << openid << ", " << expires << ", " << token << endl;

    cout << "cmp: " << (j["data"]["expires_in"] == 7200) << endl;
}
