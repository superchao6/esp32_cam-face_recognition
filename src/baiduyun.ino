#include "baiduyun.h"
// 百度云人脸搜索
String get_face(void){
    HTTPClient httpClient;
    String POST_URL = "https://aip.baidubce.com/rest/2.0/face/v3/search?access_token=24.1cd2f50fca8278b84190455984fd4e52.2592000.1654405821.282335-26147479";
    String POST_DATA = "{\"image\":\"https://img2.bemfa.com/ef1b16213a55cfa5d07ec7118c1287b4-zxc.jpg\",\"image_type\":\"URL\",\"group_id_list\":\"huaweibei_iot\",\"match_threshold\":85}";
    //配置请求地址
    httpClient.begin(POST_URL); //HTTP请求
//    Serial.print(POST_URL);
    Serial.print("[HTTP] begin...\n");
//    Serial.print(POST_DATA);
    //启动连接并发送HTTP请求。请求方法：POST。请求体信息：POST_DATA
    int httpCode = httpClient.sendRequest("POST", POST_DATA);
//    Serial.print("[HTTP] POST...\n");
    //连接失败时 httpCode时为负
    if (httpCode > 0) {
        //将服务器响应头打印到串口
//        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        //将从服务器获取的数据打印到串口
        if (httpCode == HTTP_CODE_OK) {
            const String& payload = httpClient.getString();
//            Serial.println("received payload:\n<<");
            Serial.println(payload);
//            Serial.println(">>");
            httpClient.end();

            const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 230;
            DynamicJsonBuffer jsonBuffer(capacity);
            JsonObject& root = jsonBuffer.parseObject(payload.c_str());
            JsonObject& result_user_list_0 = root["result"]["user_list"][0];
            const char* result_user_list_0_user_id = result_user_list_0["user_id"]; // "1"
            // const char* result_user_list_0_user_info = result_user_list_0["user_info"]; // ""
            // float result_user_list_0_score = result_user_list_0["score"]; // 91.637062072754
            // Serial.println(result_user_list_0_user_id);
            // return String(result_user_list_0_user_id);
            if (result_user_list_0_user_id == NULL)
            {
                return "0";
            }
            return result_user_list_0_user_id;
        }
    } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
        //关闭http连接
        httpClient.end();
        return "-1";
    }
}