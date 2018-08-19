#include <QCoreApplication>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QDebug>

#include <QtSerialPort/QSerialPort>
#include <QtSerialBus/QtSerialBus>
#include <QThread>

#include <signal.h>

#include <iostream>
using namespace std;

QSerialPort serialport;

int check_ppp_connection();
char checkInternetMode();
void setGsmPriority();
void setEthernetPriority();

void opentty();
void closetty();
bool checkPPPtty();

void turnOn_Sequence();
void start_Sequence();
void restart_Sequence();
void stop_Sequence();
int estiblish_ppp_connection(void);

int checkPing();

unsigned long  gsm_retryCount = 0;

unsigned long gsm_baseCount = 300; //30; //300;

unsigned long gsm_maxCount = 1800; //120; //1800;

void myhandler();

bool logEnable = false;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

     gsm_retryCount = gsm_baseCount;

    char c = checkInternetMode();

      if(c == 'P' || c == 'L'){

           if(c == 'L'){

              logEnable = true;
          }

          qDebug()<<"GSM Mode is Selected";
          if(logEnable)
              system("echo GSM Mode is Selected >> /root/mlogs");

           start_Sequence();

           while(1){

               if((check_ppp_connection() !=0 ) || (checkPing() != 0))
               { // PPP is down or ppp not registered or sim unplugged


                       if(logEnable)system("echo GSM is down Restarting Gsm >> /root/mlogs");
                        qDebug()<<"GSM is down"<<"Restarting Gsm";

                       restart_Sequence();

                       if(logEnable)system("echo gsm is restarted >> /root/mlogs");
                       qDebug()<<"GSM modem has restarted";


                       if(logEnable){

                           system("pppd call quectel-ppp updetach  noipdefault  noauth lock usepeerdns lcp-echo-failure 2 lcp-echo-interval 30 refuse-chap persist holdoff 5 &>> /root/mlogs");
                       }else{

                           system("pppd call quectel-ppp updetach  noipdefault  noauth lock usepeerdns lcp-echo-failure 2 lcp-echo-interval 30 refuse-chap persist holdoff 5");
                       }

                       qDebug()<<"PPP interface command is called again";

                       if(logEnable)system("echo PPP interface command is called again >> /root/mlogs");

                       for (int i = 0 ; i< 12 ; i++){

                            if(check_ppp_connection() == 0){
                                break;
                            }
                            QThread::sleep(15);
                            // wait till ppp interface is up
                       }

                       qDebug()<<"ppp is up hopefully";

                       if(logEnable)system("echo ppp is up hopfully >> /root/mlogs");

                       QThread::sleep(30);

                       qDebug()<<"setting gsm priority";
                       if(logEnable)system("echo setting gsm priority >> /root/mlogs");

                       setGsmPriority();

                       gsm_retryCount += gsm_baseCount;//*2;

                       qDebug()<<gsm_retryCount;

                       if(gsm_retryCount > gsm_maxCount)
                       {
                           gsm_retryCount = gsm_maxCount;
                       }

                }else{

                   gsm_retryCount = gsm_baseCount;

               }

               QThread::sleep(gsm_retryCount); // 5 minute base sleep

           }


       }
      else{


          //qDebug()<<"Ethernet Mode is Selected";
          system("echo Ethernet Mode is Selected >> /root/mlogs");
          setEthernetPriority();
       }

    return a.exec();
}

int check_ppp_connection()
{
    QString epath ="/sys/class/net/ppp0/operstate";
    QFile file(epath);

    if(!file.open(QFile::ReadOnly |QFile::Text))
    {
        qDebug() << " Could not open the inteface file for reading implies ppp not registered";
        system("echo Could not open the inteface file for reading implies ppp not registered >> /root/mlogs");
        return -1;
    }

    QTextStream in(&file);
    QString myText = in.readAll();
    qDebug() << myText;

    file.close();

    if(myText == "unknown\n"){

        qDebug()<<"PPP network interface is up";
        if(logEnable)
            system("echo PPP network interface is up  >>/root/mlogs");
        return 0;
    }else if(myText =="down\n") {

        qDebug()<<"PPP network interface is down";
        if(logEnable)
            system("echo PPP network interface is down  >> /root/mlogs");
        return 1;
    }else{
        qDebug()<<"PPP network interface is not registered";
        if(logEnable)
            system("echo PPP network interface is not registered yet  >> /root/mlogs");
        return -1;
    }

}

char checkInternetMode(){

    QString ipath ="/home/launchApps/Iconfig/IP_Config/internetMode.txt";

    QFile file(ipath);

    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }


    char c;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_6);

    in.readRawData(&c,1);

    return c;

}

void setGsmPriority(){

   system("route add default dev ppp0");
}

void setEthernetPriority(){

    system("route add default dev eth0");
}

void opentty(){

    serialport.setPortName("ttyS3");
    serialport.setBaudRate(QSerialPort::Baud115200);
    serialport.setDataBits(QSerialPort::Data8);
    serialport.setParity(QSerialPort::NoParity);
    serialport.setStopBits(QSerialPort::OneStop);
    serialport.setFlowControl(QSerialPort::NoFlowControl);

    if(serialport.open(QIODevice::ReadWrite)){

        qDebug()<<"Port is open Happy me";
    }else{
        qDebug()<<"Port is open Sad me";
    }

}

void closetty(){
    serialport.close();
}

bool checkPPPtty(){

    char atStatus[5]={0};
    serialport.write("AT");
    serialport.waitForReadyRead(1000);
    serialport.read(atStatus,5);
    qDebug()<<atStatus;

    return 0;
}

void turnOn_Sequence(){

    system("echo  108 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/PD12/direction");
    system("echo 1 > /sys/class/gpio/PD12/value");
    QThread::sleep(1);
    system("echo 0 > /sys/class/gpio/PD12/value");
}

void start_Sequence(){

    qDebug()<<"turning ON gsm modem for first time";

    if(logEnable)
        system("echo turning ON gsm modem for first time >>/root/mlogs");

    turnOn_Sequence();

    qDebug()<<"waiting 60s for gsm to be turned ON ";

    if(logEnable)
        system("echo waiting 60s for gsm to be turned ON >>/root/mlogs");

    QThread::sleep(60); // 1 minute sleep


    qDebug()<<"GSM modem is ON and now turning on pppd";
    if(logEnable)
        system("echo GSM modem is ON  and now turning on pppd >>/root/mlogs");

    if(logEnable){

        system("pppd call quectel-ppp updetach noipdefault noauth lock usepeerdns lcp-echo-failure 2 lcp-echo-interval 30 refuse-chap persist holdoff 5 &>> /root/mlogs");
    }else{

        system("pppd call quectel-ppp updetach noipdefault noauth lock usepeerdns lcp-echo-failure 2 lcp-echo-interval 30 refuse-chap persist holdoff 5");
    }


    qDebug()<<"waiting for ppp interface to be up (max 12 tries)";

    if(logEnable)
        system("echo waiting for ppp interface to be up >>/root/mlogs");

    for (int i = 0 ; i< 12 ; i++){

         if(check_ppp_connection() == 0){

             qDebug()<<"ppp network interface is up on "<< i<< " th try";

             if(logEnable)
                 system("echo ppp network interface is up >>/root/mlogs");

             break;
         }
         QThread::sleep(15);
         // wait till ppp interface is up
    }

    QThread::sleep(10); // 10 sec sleep

    qDebug()<<"setting gsm priority after waiting for 10sec ";
    if(logEnable)
        system("echo setting gsm priority after waiting for 10sec >>/root/mlogs");

    setGsmPriority();

}

void restart_Sequence(){

    system("echo 1 > /sys/class/gpio/PD12/value");
    QThread::sleep(1);
    system("echo 0 > /sys/class/gpio/PD12/value");
    QThread::sleep(60);

    system("echo 1 > /sys/class/gpio/PD12/value");
    QThread::sleep(1);
    system("echo 0 > /sys/class/gpio/PD12/value");
}

void stop_Sequence(){
    system("echo 1 > /sys/class/gpio/PD12/value");
    QThread::sleep(1);
    system("echo 0 > /sys/class/gpio/PD12/value");
}

int estiblish_ppp_connection(void)
{
    //noresolv need to implement
    char ppp_start_command[256] = "pppd call quectel-ppp updetach  noipdefault  noauth lock usepeerdns lcp-echo-failure 2 lcp-echo-interval 30 refuse-chap persist holdoff 5&";

    FILE *fp;

    char returnData[64];

    //printf("File to Open: %s",ppp_start_command);
    qDebug()<<"File to Open: "<<ppp_start_command;

    fp = popen(ppp_start_command, "r");

    while (fgets(returnData, 64, fp) != NULL)
    {
        if(strstr(returnData,"Connect script failed")!=NULL)
        {
            //printf("Nimish Detect :Connection Failed\n\r");
            qDebug()<<"Nimish Detect :Connection Failed\n\r";
            return -2;
        }
        else if(strstr(returnData,"+CME ERROR")!=NULL)
        {
            //printf("Nimish Detect :SIM card failed\n\r");
            qDebug()<<"Nimish Detect :SIM card failed";
            return -1;
        }

        //printf("%s", returnData);
        qDebug()<<returnData;
    }

    pclose(fp);
    return 0;
}

int checkPing(){


    if(logEnable){

        qDebug()<<"echo trying to ping";
        system("echo trying to ping >>/root/mlogs");

        if(system("ping -c2 -s1 www.google.com &>>/root/mlogs") == 0){

             qDebug()<<"ping success";
             system("echo ping success >>/root/mlogs");
            return 0;
        }else{

            qDebug()<<"ping failed";
            system("echo ping failed >>/root/mlogs");
            return 1;
        }

    }else{

        qDebug()<<"echo trying to ping";
        if(system("ping -c2 -s1 www.google.com") == 0){

             qDebug()<<"ping sucess";
            return 0;
        }else{

             qDebug()<<"ping failed";
            return 1;
        }

    }


}
