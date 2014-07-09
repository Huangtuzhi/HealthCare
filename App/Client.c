#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main()
{
	int cfd; //定义socket文件描述符
	int recbytes;
	int sin_size;
	char buffer[1024]={0};   
	struct sockaddr_in s_add,c_add;
	unsigned short portnum=0x8888; 

	printf("This is the client !\r\n");
    cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == cfd)
	{
		printf("socket error! \r\n");
		return -1;
	}
	printf("socket请求成功 !\r\n");

	/******和Server建立连接******/
	bzero(&s_add,sizeof(struct sockaddr_in));//将s_add的前4个字节置为0
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr("202.201.13.105");
    //s_add.sin_addr.s_addr= inet_addr("127.0.0.1");
	s_add.sin_port=htons(portnum);
	printf("服务器IP:%#x，端口 : %#x\r\n",s_add.sin_addr.s_addr,s_add.sin_port);

	if(-1 == connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		printf("connect error !\r\n");
		return -1;
	}
	printf("连接成功 !\r\n");
  
	//这里还要加写command的代码
	if(send(cfd,"CMD",3,0)==-1)
	{
		perror("send");
		exit(1);
	}
	
	if(-1 == (recbytes = read(cfd,buffer,1024)))
	{
		printf("read data fail !\r\n");
		return -1;
	}
	printf("Received Data:\r\n");
	buffer[recbytes]='\0';
	printf("%s\r\n",buffer);
	getchar();
	close(cfd);
	return 0;
}
