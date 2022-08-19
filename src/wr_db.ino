#include "wr_db.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
/**
 * @brief 获取数据库token
 * 
 * @return String 密钥 
 * 错误返回 0
 */
String Get_DB_Token(){
  HTTPClient httpClient;
  
  httpClient.begin(TOKEN_URL); 
  Serial.print("URL: "); Serial.println(TOKEN_URL);

  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(TOKEN_URL);

  if (httpCode == HTTP_CODE_OK) {
    String responsePayload = httpClient.getString();
    Serial.println("Server Response Payload: ");
    Serial.println(responsePayload);
    httpClient.end();
    
    const size_t capacity = JSON_OBJECT_SIZE(2) + 210;
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.parseObject(responsePayload.c_str());
    const char* access_token = root["access_token"];

    return String(access_token);
  } else {
    Serial.println("Server Respose Code: ");
    Serial.println(httpCode);
    httpClient.end();
    return "0";
  }
}

/**
 * @brief 访问数据库，上传数据
 * 
 * @param access_token 密钥
 * @param account_id 用户ID
 * @param temperature 温度
 * @param clock_flag 出勤状态
 * @param time1 时间
 */
void Write_DB_Person(String access_token,String account_id,String temperature,String clock_flag,String time1,String attendance,String late){
  HTTPClient httpClient;
  String POST_URL = "https://api.weixin.qq.com/tcb/databaseupdate?access_token="+access_token;
  String POST_DATA = "{\"env\":\"huaweibei-iot-0g4n8v1k88e3fd18\",\"query\": \"db.collection('all-people').where({account_id:" + account_id + "}).update({data:{clock_flag:'" + clock_flag + "',temperature:'" + temperature + "',time:'" + time1 +"',attendance:'"+ attendance +"',late:'"+ late +"'},})\"}";
  //配置请求地址
  httpClient.begin(POST_URL); //HTTP请求
  Serial.print(POST_URL);
  Serial.print("[HTTP] begin...\n");
  Serial.print(POST_DATA);
  //启动连接并发送HTTP请求。请求方法：POST。请求体信息：POST_DATA
  int httpCode = httpClient.sendRequest("POST", POST_DATA);
  Serial.print("[HTTP] POST...\n");
  //连接失败时 httpCode时为负
  if (httpCode > 0) {
    //将服务器响应头打印到串口
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    //将从服务器获取的数据打印到串口
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = httpClient.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
      httpClient.end();
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
    //关闭http连接
    httpClient.end();
  }
}