#include"../xbox/xjson.hpp"
#include<cstdio>

void test0(){
    printf("-----------------------------\n");
    xjson payload = {
        {"name", "xjson"},
        {"support", 14},
        {"byte", 16},
        {"something", xjson::Array({1, 2, 3.1415, false, nullptr})}
    };
    auto json_str = payload.dump();
    payload["name"] = "myxjson";
    payload["newfield"] = xjson::Array({"anything", true});
    printf("dumped json: %s\n", json_str.c_str());
    printf("after json: %s\n", payload.dump().c_str());

    // payload = xjson::parse(json_str);
    payload = xjson::parse(json_str.c_str(), json_str.size());
}


void test1(){
    printf("-----------------------------\n");
    xjson payload = {
        {"model", "gpt-4o"},
        {"max_tokens", 300},
        {"messages", xjson::Array({
            xjson::Object({
                {"role", "user"},
                {"content", xjson::Array({
                    xjson::Object({ {"type", "text"}, {"text", "Hi, this messgae is for testing image api, please reply \\\"Yes I can understand.\\\" if no error occurs."} }),
                    xjson::Object({ {"type", "image_url"}, {"image_url", xjson::Object({{"url", ""}, {"detail", "low"}})}})
                    })}
                })
            })}
    };
    payload["max_tokens"] = 4000;
    payload["min_tokens"] = 300;    // adding new key
    xjson::String& url = payload["messages"][0]["content"][1]["image_url"]["url"];
    url = "https://*****.png";

    auto json_str = payload.dump();
    printf("Dumped json:\n%s\n", json_str.c_str());
}

void test2(){
    printf("-----------------------------\n");
    const char x[] = "{\"review_id\":\"KqNvf7IaK61FFRAr26ob-w\",\"user_id\":\"NeOIp8tz0kfxtmA8YUPmJA\",\"business_id\":\"IEf7QVvtwKOKsm8tEpUNCw\",\"stars\":1,\"date\":\"2011-10-04\",\"text\":\"I'm only writing this review for the receptionist that just took my call. I'm almost in tears. I just moved here and I was simply calling because a friend had referred me to one of the Dr's there. I called to see if I could set up my first initial appointment here and stated that a current patient (gave her name) referred me. I was turned away because the specific doctor I asked for was \\\"only taking new patients by referrals from other patients\\\". Me: I just told you that I was referred by a friend that is this doc's patient already. Receptionist: \\\"I'm sorry ma'am we have to get approval first before we can see you\\\". Me: \\\"Ok, well can we proceed with approvals and let me know what other information you need?\\\" \\\"Receptionist: [ long pause] \\\"ma'am we can't.....\\\" Me: hangs up phone.... \\n\\nObviously she didn't want to be bothered. I was referred but yet still couldn't get an appointment. Frustrating to those that are new to the area. \\n\\nThat is all...\",\"useful\":14,\"funny\":3,\"cool\":0}";
    
    xjson payload2 = xjson::parse(x, sizeof(x));
    printf("Dumped json:\n%s\n", payload2.dump().c_str());

    payload2["text"] = "I am happy";
    printf("After json:\n%s\n", payload2.dump().c_str());
}

void test3(){
    printf("-----------------------------\n");
    xjson js = true;
    js = 1000;
    js = xjson::Array({1, 2, "666", 3.1415926, true, nullptr, false});
    printf("Dumped json:\n%s\n", js.dump().c_str());
}

int main(){
    test0();
    test1();
    test2();
    test3();
    return 0;
}