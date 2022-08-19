#include <Arduino.h>
#include <esp_http_client.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>
#include "FS.h"
#include "SPIFFS.h"

//显示摄像头数据
#include <esp_camera.h>
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#include "wr_db.h"
#include "baiduyun.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define FORMAT_SPIFFS_IF_FAILED true// flash

const char* ssid = "superchao";           //WIFI名称
const char* password = "zxczxczxc";     //WIFI密码
int count=0;
String sta;//打卡状态 出勤or缺勤

int Isdouhao=0,aa=6,bb=0; //aa 表示考勤时间hour   bb 表示考勤时间min
String Day_mes,Hour_mes,Min_mes,Sec_mes,Timemes,Timeme; //用于存放获取的网络时间以及更新系统的时间日期
void StrToInt(String str,int a,int b);

TaskHandle_t mainSerTask_Handle; //  main GUI 绘制任务句柄

//用户信息
char users_names[50][20];//存放用户名称
char users_times[50][10];//存放用户打卡时间
char users_kaoqinstate[50][10];//存放用户考勤状态
char users_states[50][5];//存放用户打卡状态
char users_chuqinday[50][10];//存放用户出勤天数
char users_chidaoday[50][10];//存放用户迟到天数

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8*3600; //不知道为何是4*60*60
const int   daylightOffset_sec = 4*3600;//不知道为何是4*60*60

//巴法云参数
static String httpResponseString;//接收巴法云返回信息
const char*  post_url = "https://images.bemfa.com/upload/v1/upimages.php"; // 默认上传地址
const char*  uid = "ed535bbe5e68e04d177a33f4581b656a";//用户私钥，巴法云控制台获取
const char*  topic = "AIOT";//主题名字
const char*  wechatMsg = "";//为空
const char*  wecomMsg = "";//为空
const char*  urlPath = "zxc";//图片链接值

//文件操作
String filepath,filepath_kdaily,filepath_cdaily;
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void readFile(fs::FS &fs, const char * path);
void readFileToStr(fs::FS &fs, const char * path,String *str);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void testFileIO(fs::FS &fs, const char * path);
void deleteFile(fs::FS &fs, const char * path);

//WIFI初始化
void Wifi_Connect()
{
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nConnected  ");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
//摄像头初始化
void Camera_Init(){
    Serial.println("INIT CAMERA");
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;
    //init with high specs to pre-allocate larger buffers
    if(psramFound()){
        config.frame_size = FRAMESIZE_HQVGA; // 320x240
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_HQVGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void Time_task( void *pvParameters );
void Compare_Face( void *pvParameters );

void setup() {

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200); 
    Serial2.begin(115200);
    Serial.print("Serial begin!");

    //Flash 初始化
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){ //初始化flash,若失败则重启
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    strcpy(users_names[5],"张·三");
    strcpy(users_names[4],"邹光辉");
    strcpy(users_names[3],"刘明星");
    strcpy(users_names[2],"熊·超");
    strcpy(users_names[1],"岳奕松");
    strcpy(users_names[0],"周贤超");

    Wifi_Connect(); //wifi初始化
    Camera_Init();  //摄像头初始化

    // 从互联网获取实时时间
    xTaskCreatePinnedToCore(
    Time_task     //任务函数
    ,  "Time Task"   /* 任务名字 没啥用，就是描述*/
    ,  1024*4       /*堆栈大小，单位为字节,缺省1024无法正确运行，需要增大至1024*2  */
    ,  NULL         /*作为任务输入传递的参数*/
    ,  3            /*任务的优先级Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  NULL 
    ,  1);          //  1: APP_core

    //人脸识别
    xTaskCreatePinnedToCore(
    Compare_Face     //任务函数
    ,  "Compare face"   /* 任务名字 没啥用，就是描述*/
    ,  1024*12       /*堆栈大小，单位为字节,缺省1024无法正确运行，需要增大至1024*2  */
    ,  NULL         /*作为任务输入传递的参数*/
    ,  3            /*任务的优先级Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  (TaskHandle_t* ) &mainSerTask_Handle
    ,  1);          //  1: APP_core
}

/********巴法云对接函数*********/
//巴法云http请求处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  if (evt->event_id == HTTP_EVENT_ON_DATA)
  {
    httpResponseString.concat((char *)evt->data);
  }
  return ESP_OK;
}
//巴法云推送图片
static esp_err_t take_send_photo()
{
  Serial.println("Taking picture...");
  camera_fb_t * fb = NULL;
//   esp_err_t res = ESP_OK;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  httpResponseString = "";
  esp_http_client_handle_t http_client;
  esp_http_client_config_t config_client = {0};
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;
  http_client = esp_http_client_init(&config_client);
  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);//设置http发送的内容和长度
  esp_http_client_set_header(http_client, "Content-Type", "image/jpg"); //设置http头部字段
  esp_http_client_set_header(http_client, "Authorization", uid);        //设置http头部字段
  esp_http_client_set_header(http_client, "Authtopic", topic);          //设置http头部字段
  esp_http_client_set_header(http_client, "wechatmsg", wechatMsg);      //设置http头部字段
  esp_http_client_set_header(http_client, "wecommsg", wecomMsg);        //设置http头部字段
  esp_http_client_set_header(http_client, "picpath", urlPath);          //设置http头部字段
  esp_err_t err = esp_http_client_perform(http_client);//发送http请求
  if (err == ESP_OK) {
    // Serial.println(httpResponseString);//打印获取的URL
    Serial.println("ok!");
  }
  esp_http_client_cleanup(http_client);
  esp_camera_fb_return(fb);
}

void loop() {

}

//获取时间
void printLocalTime()
{
  struct tm timeinfo;
  static int k=0;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  getLocalTime(&timeinfo);
  String maohao=" : ",ling="0";
  Min_mes = (String)timeinfo.tm_min;
  if (timeinfo.tm_hour >= 4)
  {
    Hour_mes = (String)(timeinfo.tm_hour - 4);
  }else{
    Hour_mes = (String)(24 - 4 + timeinfo.tm_hour);
  }
  if(atoi(Hour_mes.c_str()) < aa) sta = "出勤";                         // 考勤时间late_Time 设置为9点,出勤为置1
  else if(atoi(Hour_mes.c_str()) == aa && timeinfo.tm_min<bb) sta = "出勤";  // 考勤时间late_Time 设置为9点,出勤为置1
  else sta = "缺勤";

  if(atoi(Hour_mes.c_str()) < 10) Hour_mes = ling + Hour_mes;
  if(timeinfo.tm_min < 10)  Min_mes = ling + Min_mes;
  //Serial.println(&timeinfo, "First: %A, %B %d %Y %H:%M:%S");
//   Timemes = Hour_mes + maohao + kong + Day_mes;
  Timeme  = Hour_mes + maohao + Min_mes;     //用打卡时间记录
  Serial.println(Hour_mes + " : " + Min_mes);
}


void Time_task( void *pvParameters ) {
    while(1){
    printLocalTime();
    vTaskDelay(2000);    //10s更新一次时间
  }
}

void Compare_Face( void *pvParameters ) {
    
    while(1){
    vTaskDelay(20);
    take_send_photo();
    String people = get_face();
    Serial.println("People:" + people);
    if(people == "0" || people == "-1"){
        continue;
    }else{
        
        vTaskDelay(10);

        int who_num_int;
        String who_num = people;
        who_num_int = atoi(who_num.c_str());
        String timeAndstate = Timeme + "···" + sta;
        String kaoqin_days,chidao_days;
        filepath = "/"+who_num+".txt";
        filepath_kdaily = "/k" + who_num + ".txt" ;
        filepath_cdaily = "/c" + who_num + ".txt" ;
        SPIFFS.remove(filepath.c_str());

        listDir(SPIFFS, "/", 0);
        writeFile(SPIFFS,filepath.c_str(),&timeAndstate.c_str()[0]);//写时间和状态到flash
        readFileToStr(SPIFFS,filepath_kdaily.c_str(),&kaoqin_days);//读取当前用户的打卡天数
        int kaoqin_days_int = atoi(kaoqin_days.c_str());

        readFileToStr(SPIFFS,filepath_cdaily.c_str(),&chidao_days);//读取当前用户的打卡天数
        int chidao_days_int = atoi(chidao_days.c_str());
        // 尚未判断是否重复打卡
        if(sta == "出勤")
        {      
            kaoqin_days_int++; 
            kaoqin_days = (String)kaoqin_days_int; 
            strcpy(users_chuqinday[who_num_int],kaoqin_days.c_str());//sta
            writeFile(SPIFFS,filepath_kdaily.c_str(),&kaoqin_days.c_str()[0]);//写flash
        }
        else
        {
            chidao_days_int++; 
            chidao_days = (String)chidao_days_int;
            strcpy(users_chidaoday[who_num_int],chidao_days.c_str());//sta
            writeFile(SPIFFS,filepath_cdaily.c_str(),&chidao_days.c_str()[0]);//写flash
        }
        //o0周·超···01 : 36···出勤;周·超·····12·····1;
        String send = "o" + String(count) + String(users_names[who_num_int-1]) + "···" + Timeme + "···" + sta + ";" + String(users_names[who_num_int-1]) + "·····" + kaoqin_days + "·····" + chidao_days + ";";

        Serial2.println(send);

        vTaskDelay(10);

        String db_token = Get_DB_Token();//获取数据库密钥
        Write_DB_Person(db_token,people,"正常",sta,Timeme,kaoqin_days,chidao_days);//写个人打卡情况

        count++;
    }
  }
}
