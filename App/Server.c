#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
int main()
{
	int sfp,nfp;
	struct sockaddr_in s_add,c_add;
	int sin_size;
	unsigned short portnum=0x8888;
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
	
	while(1)
	{
		int numbytes;
		char buff[30];
		memset(buff,0,30);

		sin_size = sizeof(struct sockaddr_in);
		nfp = accept(sfp, (struct sockaddr *)(&c_add), &sin_size);
		if(-1 == nfp)
		{
			printf("accept fail !\r\n");
			return -1;
		}

		if((numbytes=recv(nfp,buff,sizeof(buff),0))==-1)
		{
			perror("receive error");
			exit(1);
		}
		printf("Comitting Command:%s",buff);

		if(-1 == write(nfp,"我是黄逸，这里用来传递回去血压数据\r\n",60))
		{ 
			printf("write fail!\r\n");
			return -1;
		}
		printf("所测数据已发送\n");
		close(nfp);
	}

	close(sfp);
	return 0;
}
