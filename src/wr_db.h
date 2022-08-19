#ifndef WR_DB
#define WR_DB
#include "wr_db.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

const String APPID = "wx629554c6089ed403";
const String APPSECRET = "7be20c707fef6b62e318363a74552b99";
String TOKEN_URL = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid="+APPID+"&secret="+APPSECRET;

String Get_DB_Token();//获取数据库密钥
void Write_DB_Person(String access_token,String account_id,String temperature,String clock_flag,String time1,String attendance,String late);//写个人打卡情况

#endif