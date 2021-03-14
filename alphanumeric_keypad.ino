/*

  Project : 4*4 Alphanumeric Keypad
     File : alphanumeric_keypad.ino
   Author : Felix Nie
  Version : 1.0
  Updated : Dec 31, 2016
     Repo : https://github.com/felixnie/Alphanumeric-Keypad
 Encoding : UTF-8
   
========================Instructions========================

    This program demostrates how to input alphanumeric char-
    acters via 4*4 keypads.
    Here are the characters included:

    0 1 2 3 4 5 6 7 8 9
    
    a b c d e f g h i j k l m n o p q r s t u v w x y z
    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
    
    @   $   %   |   !                 (   )   \   |   /
    ^   &   ~   |   ?                 [   ]   <   |   *
    :   ;   `   |   '                 {   }   >   |   -
    .   #       |   "                     _ space |   +

***********************************************************/

#include <Keypad.h>
#include <string.h>
#define N 50

const byte ROWS = 4;
const byte COLS = 4;
char myKeys[ROWS][COLS] = {
    {'1','2','3','A'},//退格
    {'4','5','6','B'},//大写锁定
    {'7','8','9','C'},//时间设置
    {'L','0','R','D'} //发送
};
byte rowPins[ROWS] = {3, 2, 9, 8};
byte colPins[COLS] = {7, 6, 5, 4}; 

char Key;//在每次loop开始时，获取按键状态

char str[N];//输入字符
boolean funcEnable = true ;//功能键开关
boolean readytoSend = false ;//准备就绪标志
boolean capsLock = false ;//
byte pos = 0 ;//光标位置
char lastKey;//保存上一次按键信息，在按键按下时更改状态
Keypad myKeypad = Keypad( makeKeymap(myKeys), rowPins, colPins, ROWS, COLS); //键盘初始化

byte setHour, setMin, setSec;

void setup(){
  Serial.begin(9600);//打开串口
}
  
void loop(){
    Key = myKeypad.getKey();//获取按键状态
    
    if( (Key>='0' && Key<='9') && (lastKey!='L' && lastKey!='R') ){//数字键状态下直接输入
        str[pos]=Key;
        pos++;//光标右移
        lastKey=Key;//保存按键信息以供功能键参考
        funcEnable=false;//将功能键锁定
    }
    if( (Key>='0' && Key<='9') && (lastKey=='L' || lastKey=='R') ){//如果按下数字键后发现上一个状态为L/R状态
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
        }//switch(Key)结束
        
        funcEnable=true;//输入字符，重启功能键
        lastKey='\0';
    }
    
    
    if( Key=='L' || Key=='R' ){//符号键判断
        if( Key=='L' )
            str[pos]='.';
        else
            str[pos]=' ';
        pos++;//光标右移
        
        lastKey=Key;//保存为L/R以供功能键参考
        funcEnable=false;//将功能键锁定
    }
        
    if( Key>='A' && Key<='D' ){
        if( funcEnable==true ){
            switch (Key) {
                
                case 'A'://光标后移一位，并把该位的字符清空
                    if( pos>=1 ){
                        pos--;
                        str[pos]='\0';
                    }
                    break;
                    
                case 'B'://大写锁定状态反转
                    if( capsLock==false )
                        capsLock=true;
                    else
                        capsLock=false;
                    break;
                    
                case 'C'://时间设置
                    if( strlen(str)==8 ) {
                        setHour= ((byte)str[0])*10 + (byte)str[1];
                        setMin = ((byte)str[3])*10 + (byte)str[4];
                        setSec = ((byte)str[6])*10 + (byte)str[7];
                        rtc.setTime(setHour, setMin, setSec);//设置时间（可能有格式问题）
                        for(;pos>=0;pos--)
                            str[pos]='\0';//清空字符串
                        Serial.println(rtc.getTimeStr());
                    }
                    break;
                    
                case 'D'://装载待发送字符
                    readytoSend=true;
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
                            str[pos-1]='\?';//问号，使用转义字符
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
                            str[pos-1]='\'';//单引号，使用转义字符
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
                            str[pos-1]='\"';//双引号，使用转义字符
                            break;
                        case 'R' :
                            str[pos-1]='+';
                            break;
                    }
                    break;
                    
            }//switch(Key)结束
        }//else结束   
    }//功能键相关代码结束
    
    Serial.print("srt==");
    Serial.print(str);
    Serial.print(' ');
    Serial.print("funcEnable==");
    Serial.print(funcEnable);
    Serial.print(' ');
    Serial.print("capsLock==");
    Serial.print(capsLock?'1':'0');
    Serial.print(' ');
    Serial.print("readytoSend==");
    Serial.print(readytoSend);
    Serial.print(' ');
    Serial.print("position==");
    Serial.print(pos);

    Serial.println();
    
}
