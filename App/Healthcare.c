#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "BloodPressure.c"

unsigned char AppCommand[10]; //全局变量，用来存储Andriod App发来的命令
int sfp,nfp;
struct sockaddr_in s_add,c_add;
int sin_size;
unsigned short portnum=0x8888;

int ConnectToApp(void)
{
	printf("This is the server !\r\n");
	sfp = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sfp)
	{
		printf("socket Error ! \r\n");
		return -1;
	}

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr=htonl(INADDR_ANY);
	s_add.sin_port=htons(portnum);

	if(-1 == bind(sfp,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		printf("bind fail !\r\n");
		return -1;
	}

	if(-1 == listen(sfp,7))
	{
		printf("listen fail !\r\n");
		return -1;
	}
	printf("监听数据中...\r\n");
}

#define Motor 0
#define Valve 1
#define ON    0
#define OFF   1
#define sample_num 3800
#define bias 500

short ReceiveData[sample_num];
double BUFER[sample_num];
double HighOut[sample_num];
double LowOut[sample_num];

int main(int argc,char **argv)
{   
	int fd0=open("/dev/GPIOs",0);
    int fd1=open("/dev/myadc",0);
    
	double ps,pd,shousuo,shuzhang;
	int SampCount=0;
	int Trace[2]={0};
	int peak[256];
	int cnt;
	int F=0;
    int UserFlag=0;//标志是否返回用户数据
    char UserData[48];
  
	if(fd0<0){
		fd0=open("/dev/GPIOs",0);
	}
	if(fd0<0){
		perror("Open device GPIOs");
		exit(1);
	}

	if(fd1<0){
		fd1=open("/dev/myadc",0);
	}
	if(fd1<0){
		perror("Open device ADCs");
		exit(1);
	}

	ConnectToApp();//连接App

	while(1)
 {
		int numbytes;
		memset(AppCommand,0,4);

		sin_size = sizeof(struct sockaddr_in);
		nfp = accept(sfp, (struct sockaddr *)(&c_add), &sin_size);
		if(-1 == nfp)
		{
			printf("accept fail !\r\n");
			return -1;
		}

		if((numbytes=recv(nfp,AppCommand,sizeof(AppCommand),0))==-1)
		{
			perror("receive error");
			exit(1);
		}
        
 if(strcmp(AppCommand,"CMD")==0)
  {
	printf("正在进行血压测量。请您稍等。\n\r");
	ioctl(fd0,ON,Valve);//ioctl(file,state,device)注意和驱动顺序对着
    ioctl(fd0,ON,Motor);

	//以下代码段为读取AD数据，对压力值进行检测，不对数据进行存储。当充气达到一定压力时，停止充气。
	for(;;)
	{
		char buffer[20];
		int len1 = read(fd1, buffer, sizeof buffer-1);
		if (len1 > 0) {
			buffer[len1] = '\0';
			int value = 0;
			sscanf(buffer, "%d", &value);
			if(value>750) break;
		}
	}
	
	ioctl(fd0,OFF,Valve);
    ioctl(fd0,OFF,Motor);
	/*因为ADC驱动中是以char字符数组（int+\n)的格式copy_to_user到用户空间的，这里需要把串口流中的字符数组读到
	 * buffer中，再恢复为short类型的ADC值（因为是10位ADC，范围为0～1024）。*/
	int index=0;
	for(;;)
	{   
		char buffer[20];
		static int DownCnt=0;
		int len2 = read(fd1, buffer, sizeof buffer-1);
		if (len2 > 0) {
			buffer[len2] = '\0';
			int value=0;
			sscanf(buffer, "%d", &value);
			DownCnt++;
			if(DownCnt==1)
			{
		    ReceiveData[index++]=value;
        	printf("ADC Value:%d %d\n",index ,value);
			DownCnt=0;
			}
			if(index==sample_num)
			{
	//			close(fd1);
				break;
			}
		}
		else
		{
			perror("read ADC device:");
			return 1;
		}
	}

    filter(ReceiveData,BUFER,sample_num);
	butter_high(BUFER,HighOut,sample_num);
    butter_low(HighOut,LowOut,sample_num);
    DIFF(&LowOut[bias],BUFER,sample_num-bias);
    cnt=find_max(BUFER,peak,sample_num-2-bias);
    printf("cnt numbers %d\n",cnt);
	F= FindPeak(&LowOut[bias],peak,Trace,cnt);
	printf("Related Points %d %d\n",Trace[0],Trace[1]);
	ps=ReceiveData[Trace[0]+bias];
	pd=ReceiveData[Trace[1]+bias];
	shousuo=((ps/1024*3.3/5-0.04)/0.018)*7.5006168;
	shuzhang=((pd/1024*3.3/5-0.04)/0.018)*7.5006168;
	printf("收缩压：%f \n",shousuo);
	printf("舒张压: %f \n",shuzhang);
	printf("脉  搏：%d \n",F-30);
    sprintf(UserData,"收缩压:%7.3lf |舒张压:%7.3lf |脉搏:%d",shousuo,shuzhang,F-30);    	
    UserFlag=1;
  }

 if(UserFlag==1)
 {
   UserFlag=0;
   if(-1 == write(nfp,UserData,48))
   {
	   printf("write fail!\r\n");
	   return -1;
   }
   printf("所测数据已发送\n");
   close(nfp);
 }
 else
 printf("控制命令非法，请按说明操作.");
}//while循环
}//总循环

