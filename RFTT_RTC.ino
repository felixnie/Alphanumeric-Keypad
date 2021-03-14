/***********************************************************

  Project : Radio Freqency Text Transceiver
     File : RFTT_RTC.ino
  Version : 0.9
  Updated : Dec 31, 2016
     Repo : https://github.com/felixnie/Alphanumeric-Keypad
 Encoding : UTF-8

========================Instructions========================
    
    0 1 2 3 4 5 6 7 8 9
    
    a b c d e f g h i j k l m n o p q r s t u v w x y z
    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
    
    @   $   %   |   !                 (   )   \   |   /
    ^   &   ~   |   ?                 [   ]   <   |   *
    :   ;   `   |   '                 {   }   >   |   -
    .   #       |   "                     _ space |   +
    
=====================Wiring Arrangement=====================

|  nRF24L01  |     RTC     |    OLED    |      Keypad      |
    
|   CE = D9  |   CE = D4   |  SCL = A?  | ROWS A0,A1,A2,A3 |
|  CSN = D10 |   IO = D3   |  SDA = A?  | COLS D8,D7,D6,D5 |
| MOSI = D11 |  SCLK = D2  |
| MISO = D12
|  SCK = D13
|  IRQ = NC

***********************************************************/
#include <Keypad.h>
#include <DS1302.h>                 // RTC library
#include <EEPROM.h>
#include "SPI.h"
#include "Mirf.h"                   // Mirf library
#include "nRF24L01.h"               // nRF24L01 library
#include "MirfHardwareSpiDriver.h"  // nRF24L01 library

#include <U8glib.h> // OLED library
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
// SSD1306 chipset + I2C protocol

#define N 22 // buffer length

// Real-Time Clock Initialization
// rtc(chip_enable_pin, input_output_pin, serial_clock_pin);
DS1302 rtc(4,3,2);

Time t; // Time object t
int setHour,setMin,setSec;

const byte ROWS = 4;
const byte COLS = 4;
char myKeys[ROWS][COLS] = {
                        // Function Keys:
    {'1','2','3','A'},  // Backspace
    {'4','5','6','B'},  // Caps Lock
    {'7','8','9','C'},  // Time Setting
    {'L','0','R','D'}   // Send
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {8, 7, 6, 5};

// Keypad Initialization
Keypad myKeypad = Keypad( makeKeymap(myKeys), rowPins, colPins, ROWS, COLS);

char Key; // 在每次loop开始时，获取按键状态
char lastKey; // 保存上一次按键信息，在按键按下时更改状态
char str[N]; // 输入字符
boolean funcEnable = true ; // 功能键开关
boolean capsLock = false ;//
byte pos = 0 ; // 光标位置

String newMsg; // 每次收信都将数据存入newMsg

void changeEEsend() { // 单击发送键时，修改EEPROM内容
    String timeStr;
    timeStr=rtc.getTimeStr();
    int i;
    for( i=30;i<38;i++) // 将timeStr 0-7位取出，存入时间对应的EEPROM
        EEPROM.put(i,timeStr[i-30]);
    for( i=39;i<60;i++ ) // 将EEPROM部分清空
        EEPROM.put(i,str[i-39]);
}

void changeEEget() {
    String timeStr;
    timeStr=rtc.getTimeStr();
    int i;
    for( i=0;i<8;i++) // 将timeStr 0-7位取出，存入时间对应的EEPROM
        EEPROM.put(i,timeStr[i]);
    for( i=9;i<30;i++ ) // 将接收到的信息存入消息对应的EEPROM
        EEPROM.put(i,newMsg[i-9]);
    //for( ;i<30;i++ )
        //EEPROM.put(i,' ');
}

void printEE(){ // 取出EEPROM内容
    int i;
    char inTime[9]; // 第一行，收件时间
    char inMsg[N]; // 第二行，收件内容
    char outTime[9]; // 第三行，发件时间
    char outMsg[N]; // 第四行，发件内容
    for( i=0;i<8;i++) // 将EEPROM 0-7位取出，存入时间临时字符数组inTime
        EEPROM.get(i, inTime[i]);
    for( i=9;i<30;i++) // 将EEPROM 10-29位取出，存入消息临时字符数组inMsg
        EEPROM.get(i, inMsg[i-9]);
    for( i=30;i<38;i++) // 将EEPROM 30-37位取出，存入时间临时字符数组outTime
        EEPROM.get(i, outTime[i-30]);
    for( i=39;i<60;i++) // 将EEPROM 40-59位取出，存入消息临时字符数组outMsg
        EEPROM.get(i, outMsg[i-39]);
    inTime[8]='\0';
    inMsg[19]='\0';
    outTime[8]='\0';
    outMsg[19]='\0';
    u8g.firstPage(); // 开始绘制
    do {
        u8g.drawLine(0, 25, 128, 25); // 绘制分割线
        u8g.drawLine(0, 51, 128, 51);
        u8g.drawStr(0, 11, inTime); // 分别打印
        u8g.drawStr(0, 24, inMsg);
        u8g.drawStr(0, 37, outTime);
        u8g.drawStr(0, 50, outMsg);
        u8g.drawStr(0, 63, str); // 打印当前编辑的消息
    } while (u8g.nextPage());

}  
    
void sendMsg(){ //  sends a string via the nRF24L01
    int i;
    char everything[62];
    for(i=0;i<60;i++)
        everything[i]=' '; // 0-59位准备存入内容
    everything[60]='_'; // 第60位加入标记，作为结尾发送
    everything[61]='\0'; // 第61位加入标记，表明发送结束
    for( i=0;i<8;i++) // 将EEPROM 0-7位取出，存入时间临时字符数组inTime
        EEPROM.get(i, everything[i]);
    for( i=9;i<30;i++) // 将EEPROM 10-29位取出，存入消息临时字符数组inMsg
        EEPROM.get(i, everything[i]);
    for( i=30;i<38;i++) // 将EEPROM 30-37位取出，存入时间临时字符数组outTime
        EEPROM.get(i, everything[i]);
    for( i=39;i<60;i++) // 将EEPROM 40-59位取出，存入消息临时字符数组outMsg
        EEPROM.get(i, everything[i]);
    for( i=0;i<60;i++) // 将EEPROM 40-59位取出，存入消息临时字符数组outMsg
        if( everything[i]=='\0' )
            everything[i]=' ';
    
    for( i=0;everything[i]!='\0';i++){ // 调用Mirf库函数进行发送
        byte c; // 创建临时byte类型变量
        c = everything[i]; // 获取everything[i]的ASCII编码
        Mirf.send(&c); // 发送字符
        while( Mirf.isSending() );
        //delay(10); // 修改此处可降低丢包率
    }
    Serial.println(everything);
}

void getMsg(){
    byte c; // byte类型的临时字符
    while(Mirf.dataReady()){ // 检测消息是否已准备好
        Mirf.getData(&c); // 获取之
        char letter = char(c);
        if( letter!='_' ) 
            newMsg = String(newMsg+letter);
        else {
            changeEEget(); // 改变EEPROM中的接收信息
            Serial.println(newMsg);
            delay(500);
            sendMsg();
            printEE();
            newMsg=""; // 清空newMsg字符串
        }
    }
}  

void funcKeypad(){
    
    Key = myKeypad.getKey(); // 获取按键状态
    
    if( (Key>='0' && Key<='9') && (lastKey!='L' && lastKey!='R') ){ // 数字键状态下直接输入
        str[pos]=Key;
        pos++; // 光标右移
        lastKey=Key; // 保存按键信息以供功能键参考
        funcEnable=false; // 将功能键锁定
    }
    if( (Key>='0' && Key<='9') && (lastKey=='L' || lastKey=='R') ){ // 如果按下数字键后发现上一个状态为L/R状态
        switch (lastKey) {
            case 'L':
                switch (Key) {
                    case '1' :
                        str[pos-1]='@';
                        break;
                    case '2' :
                        str[pos-1]='$';
                        break;
                    case '3' :
                        str[pos-1]='%';
                        break;
                    case '4' :
                        str[pos-1]='^';
                        break;
                    case '5' :
                        str[pos-1]='&';
                        break;
                    case '6' :
                        str[pos-1]='~';
                        break;
                    case '7' :
                        str[pos-1]=':';;
                        break;
                    case '8' :
                        str[pos-1]=';';
                        break;
                    case '9' :
                        str[pos-1]='`';
                        break;
                    case '0' :
                        str[pos-1]='#';
                        break;
                }
                break;
                
            case 'R':
                switch (Key) {
                    case '1' :
                        str[pos-1]='(';
                        break;
                    case '2' :
                        str[pos-1]=')';
                        break;
                    case '3' :
                        str[pos-1]='\\';
                        break;
                    case '4' :
                        str[pos-1]='[';
                        break;
                    case '5' :
                        str[pos-1]=']';
                        break;
                    case '6' :
                        str[pos-1]='<';
                        break;
                    case '7' :
                        str[pos-1]='{';
                        break;
                    case '8' :
                        str[pos-1]='}';
                        break;
                    case '9' :
                        str[pos-1]='>';
                        break;
                    case '0' :
                        str[pos-1]='_';
                        break;
                }
                break;
        } // switch(Key)结束
        
        funcEnable=true; // 输入字符，重启功能键
        lastKey='\0';
    }
    
    
    if( Key=='L' || Key=='R' ){ // 符号键判断
        if( Key=='L' )
            str[pos]='.';
        else
            str[pos]=' ';
        pos++; // 光标右移
        
        lastKey=Key; // 保存为L/R以供功能键参考
        funcEnable=false; // 将功能键锁定
    }
        
    if( Key>='A' && Key<='D' ){
        if( funcEnable==true ){
            switch (Key) {
                
                case 'A': // 光标后移一位，并把该位的字符清空
                    if( pos>=1 ){
                        pos--;
                        str[pos]='\0';
                    }
                    break;
                    
                case 'B': // 大写锁定状态反转
                    if( capsLock==false )
                        capsLock=true;
                    else
                        capsLock=false;
                    break;
                    
                case 'C': // 时间设置
                    if( strlen(str)==8 && str[2]==':' && str[5]==':' ) {
                        setHour= (str[0]-'0')*10 + str[1]-'0';
                        setMin = (str[3]-'0')*10 + str[4]-'0';
                        setSec = (str[6]-'0')*10 + str[7]-'0';
                        rtc.setTime(setHour, setMin, setSec); // 设置时间
                        for( pos=N;pos>0;pos-- )
                            str[pos-1]='\0'; // 清空字符串
                    }
                    break;
                    
                case 'D': // 发送字符
                    changeEEsend(); // 改变EEPROM中的发送信息
                    sendMsg(); // 全盘发送
                    for( pos=N;pos>0;pos-- )
                        str[pos-1]='\0'; // 清空字符串，光标归零
                    break;
            }
        
        }
        else {
            funcEnable=true;
            switch (Key) {
                case 'A':
                    switch (lastKey) {
                        case '2' :
                            str[pos-1]=(capsLock?'A':'a');
                            break;
                        case '3' :
                            str[pos-1]=(capsLock?'D':'d');
                            break;
                        case '4' :
                            str[pos-1]=(capsLock?'G':'g');
                            break;
                        case '5' :
                            str[pos-1]=(capsLock?'J':'j');
                            break;
                        case '6' :
                            str[pos-1]=(capsLock?'M':'m');
                            break;
                        case '7' :
                            str[pos-1]=(capsLock?'P':'p');
                            break;
                        case '8' :
                            str[pos-1]=(capsLock?'T':'t');
                            break;
                        case '9' :
                            str[pos-1]=(capsLock?'W':'w');
                            break;
                        case 'L' :
                            str[pos-1]='!';
                            break;
                        case 'R' :
                            str[pos-1]='/';
                            break;
                    }
                    break;
                    
                case 'B':
                    switch (lastKey) {
                        case '2' :
                            str[pos-1]=(capsLock?'B':'b');
                            break;
                        case '3' :
                            str[pos-1]=(capsLock?'E':'e');
                            break;
                        case '4' :
                            str[pos-1]=(capsLock?'H':'h');
                            break;
                        case '5' :
                            str[pos-1]=(capsLock?'K':'k');
                            break;
                        case '6' :
                            str[pos-1]=(capsLock?'N':'n');
                            break;
                        case '7' :
                            str[pos-1]=(capsLock?'Q':'q');
                            break;
                        case '8' :
                            str[pos-1]=(capsLock?'U':'u');
                            break;
                        case '9' :
                            str[pos-1]=(capsLock?'X':'x');
                            break;
                        case 'L' :
                            str[pos-1]='\?'; // 问号，使用转义字符
                            break;
                        case 'R' :
                            str[pos-1]='*';
                            break;
                    }
                    break;
                    
                case 'C':
                    switch (lastKey) {
                        case '2' :
                            str[pos-1]=(capsLock?'C':'c');
                            break;
                        case '3' :
                            str[pos-1]=(capsLock?'F':'f');
                            break;
                        case '4' :
                            str[pos-1]=(capsLock?'I':'i');
                            break;
                        case '5' :
                            str[pos-1]=(capsLock?'L':'l');
                            break;
                        case '6' :
                            str[pos-1]=(capsLock?'O':'o');
                            break;
                        case '7' :
                            str[pos-1]=(capsLock?'R':'r');
                            break;
                        case '8' :
                            str[pos-1]=(capsLock?'V':'v');
                            break;
                        case '9' :
                            str[pos-1]=(capsLock?'Y':'y');
                            break;
                        case 'L' :
                            str[pos-1]='\''; // 单引号，使用转义字符
                            break;
                        case 'R' :
                            str[pos-1]='-';
                            break;
                    }
                    break;
                    
                case 'D':
                    switch (lastKey) {
                        case '7' :
                            str[pos-1]=(capsLock?'S':'s');
                            break;
                        case '9' :
                            str[pos-1]=(capsLock?'Z':'z');
                            break;
                        case 'L' :
                            str[pos-1]='\"'; // 双引号，使用转义字符
                            break;
                        case 'R' :
                            str[pos-1]='+';
                            break;
                    }
                    break;
                    
            } // switch(Key)结束
        } // else结束   
    } // 功能键相关代码结束
    if(Key)
        printEE();
    /*if (Key){ // 当Key获得值时更新屏幕的str区域
        u8g.firstPage(); // 开始绘制
        do {
            u8g.drawStr(0, 63, str); // 打印当前编辑的消息
        } while (u8g.nextPage());
    }*/
}

static unsigned char u8g_uestc[] U8G_PROGMEM =  // UESTC
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x7F, 0x70, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xE0, 0x1C, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x80, 0x07, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x60, 0xC0, 0x1C, 0x10, 0x00, 0x00, 0x00, 0x07, 0xC0, 0x08, 0x00, 0x08, 0x00, 0x01, 0x00, 0x09, 0x40, 0x60, 0x38, 0x10, 0x80, 0x01, 0xC0, 0x04, 0x20, 0x08, 0x40, 0x08, 0x00, 0x01, 0x20, 0x05, 0x40, 0x20, 0x60, 0x10, 0x80, 0x01, 0x20, 0x02, 0x00, 0x0A, 0x40, 0x08, 0x00, 0x01, 0x60, 0x0E, 0xC0, 0x1E, 0x40, 0x08, 0x80, 0x0F, 0x00, 0x03, 0xC0, 0x0C, 0x40, 0x1C, 0x00, 0x09, 0xC0, 0x33, 0x80, 0x3F, 0x80, 0x08, 0xF0, 0x19, 0x00, 0x3E, 0x78, 0x0A, 0xC0, 0x0F, 0x00, 0x0F, 0x20, 0x62, 0xA2, 0x21, 0xDE, 0x67, 0xD0, 0x1B, 0xC0, 0x03, 0xE0, 0x0E, 0x70, 0x04, 0xE0, 0x07, 0xC8, 0x1D, 0xA2, 0x6D, 0x02, 0x95, 0xD0, 0x09, 0x78, 0x02, 0x50, 0x7C, 0x50, 0x1E, 0xE0, 0x01, 0x48, 0x02,
  0xA2, 0x71, 0x0E, 0x13, 0xE0, 0x0B, 0x08, 0x02, 0xC8, 0x0B, 0x40, 0x09, 0x80, 0x03, 0x00, 0x3E, 0xA2, 0x6D, 0x10, 0x93, 0xE0, 0x0F, 0x00, 0x02, 0x4C, 0x08, 0x70, 0x0E, 0x80, 0x06, 0xE0, 0x03, 0x1C, 0x33, 0x9E, 0x65, 0x80, 0x81, 0x00, 0x02, 0x40, 0x08, 0x58, 0x0E, 0xC0, 0x0C, 0x10, 0x02, 0x80, 0x3E, 0xC0, 0x08, 0x00, 0xC1, 0x00, 0x02, 0x40, 0x10, 0xC8, 0x33, 0x60, 0x30, 0x00, 0x02, 0xC0, 0x30, 0x60, 0x18, 0x00, 0xFE, 0x00, 0x02, 0x00, 0x10, 0x60, 0xF0, 0x38, 0xF0, 0x01, 0x02, 0x40, 0x70, 0x30, 0x10, 0x00, 0x00, 0x80, 0x03, 0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x80, 0x03, 0x60, 0xC0, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x0F, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x07, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x1F, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xE0, 0x3C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x7F, 0xF0, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void setup(){

    int x=0;
    do {
        x = x + 8;
        u8g.firstPage();  // 开机动画
        do {
            u8g.setFont(u8g_font_gdb12);
            u8g.drawStr(20, 20, "RF Chatter");
            u8g.drawXBMP( 0, 32, 128, 64, u8g_uestc);
            u8g.drawHLine(0, 0, 2 * x);
            u8g.drawHLine( 128 - 2 * x, 63, 128);
            u8g.drawVLine(0, 0, x);
            u8g.drawVLine(127, 64 - x, 64);
        } while (u8g.nextPage());
    } while (x < 64);
    u8g.setFont(u8g_font_timB10); // 设置之后的字体
    
    rtc.halt(false); // 将时钟芯片设置为run-mode
    rtc.writeProtect(false); // 关闭RAM的写保护
    
    //rtc.setDOW(MONDAY);        // Set Day-of-Week to MONDAY
    //rtc.setTime(20, 0, 0);     // Set the time to 20:00:00 (24hr format)
    //rtc.setDate(2, 1, 2017);   // Set the date to January 2nd, 2017
    
    Mirf.csnPin = 10;
    Mirf.cePin = 9;
    Mirf.spi = &MirfHardwareSpi;
    Mirf.init(); // 初始化nRF24L01                   
    Mirf.setRADDR((byte *)"RX_01");
    Mirf.setTADDR((byte *)"TX_01"); // 注意：另一端设置应与此处相反
    Mirf.payload = 1;
    Mirf.channel = 118;
    Mirf.config();
    
    Serial.begin(9600); // 打开串口，用于测试

}
  
void loop(){
    
    //t = rtc.getTime(); // Get data from the DS1302
    funcKeypad(); // 键盘函数
    getMsg(); // 收信函数
    /*
    Serial.print("str==");
    Serial.print(str);
    Serial.print(' ');
    Serial.print("funcEnable==");
    Serial.print(funcEnable);
    Serial.print(' ');
    Serial.print("capsLock==");
    Serial.print(capsLock?'1':'0');
    Serial.print(' ');
    Serial.print("position==");
    Serial.print(pos);
    Serial.print(' ');
    Serial.print("time==");
    Serial.print(rtc.getTimeStr());
    Serial.println();
    */
}
