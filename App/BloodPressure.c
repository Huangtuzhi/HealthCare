#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <malloc.h>
/*
double BUFER[sample_num];
double filterOut[sample_num];
double HighOut[sample_num];
double LowOut[sample_num];
short ReceiveData[sample_num];//从txt文件导入数组，动态开辟内存的指针
short ReceiveData_L[sample_num];//从txt文件导入的直流分量。实际上是没有低通滤波的，有纹波干扰。
*/
void filter(short*	in,double*	out,int	length)
{
	int i;
	for(i=0;i<3;i++) *(out+i)=*(in+i);
	for(i=3;i<length-4;i++){
		*(out+i)=(double)((*(in-3+i)+*(in-2+i)+*(in-1+i)+*(in+i)+*(in+1+i)+*(in+2+i)+*(in+3+i))/7.0);
	}
}


#define	A1_H	1.0                       //Matlab算得高通滤波器的传递函数系数
#define A2_H	-1.97334424978130
#define A3_H	0.973694871976315
#define	B1_H	0.986759780439403
#define B2_H	-1.97351956087881
#define B3_H	0.986759780439403
void butter_high(double* in,double*	out,int	length)
{
	int i;
	out[0]=0;
	out[1]=0;
	for(i=2;i<length-1;i++){
		out[i]=-1*(A2_H*out[i-1]+A3_H*out[i-2])+B1_H*in[i]+B2_H*in[i-1]+B3_H*in[i-2];
	}
}

#define	A1_L	1.0
#define A2_L	-1.71283790326594
#define A3_L	0.749180967484159
#define	B1_L	0.00908576605455536
#define B2_L	0.0181715321091107
#define B3_L	0.00908576605455536
void butter_low(double*	in,double*	out,int	length)
{
	int i;
	out[0]=0;
	out[1]=0;
	for(i=2;i<length-1;i++)
		out[i]=-1*(A2_L*out[i-1]+A3_L*out[i-2])+B1_L*in[i]+B2_L*in[i-1]+B3_L*in[i-2];
}

void DIFF(double*	in,double*	out,int	length){
	int i;
	for(i=0;i<length-2;i++)
		out[i]=in[i+1]-in[i];
}

#define ks1  0.4
#define ks2  0.68
#define ks   0.58
#define kd1  0.45
#define kd2  0.85
#define kd   0.77
int find_max(double* in,int* out,int length)
{
	int i,j;
  double Max,Min,Temp=0;
	int WaveStart[1000];
	int MaxPoint[1000];

	int cnt=0,cnt1=0;
	double G;//判断点
	int range=1;//the scope of searching
    //for(i=0;i<1000;i++)
	//	printf("%f ",in[i]);
	
	//求得差分最大最小值
	Max=Min=0;
	for(j=0;j<length;j++)
	{
		if(in[j]>Max)
			 Max=in[j];
		if(in[j]<Min)
			Min=in[j];
	}
	G=(Max-Min)*0.36;
  
	for(i=5;i<length-5;)
	{
		Temp=in[i-5];
		for(j=i-5;j<=i+5;j++)
		{
			if(in[j]>Temp)
				{
					Temp=in[j];
				}
		}
		if(Max-in[i]<=G&&in[i]==Temp)
		{
			WaveStart[cnt++]=i;
			i=i+90;
		}
		else 
			i++;
}

  for(i=0;i<cnt;i++)
  {
	//int flag=1;
	 range=1;
	 while(1)
	 { 
  		if(in[WaveStart[i]+range-1]>0 && in[WaveStart[i]+range+1]<0)
  		{
			MaxPoint[cnt1++]=WaveStart[i]+range;
			break;
		}
  		else 
			range++;
    }
	memcpy(out,MaxPoint,cnt*sizeof(int));
  } 
  return cnt1-1;
}


/*int ProcessData(void)
{
FILE *fp=fopen("AC.txt","r");
int i;
for(i=0;!feof(fp);i++){
//	ReceiveData=(double*)(ReceiveData ? realloc(ReceiveData,sizeof(double)*(i+1)):malloc(sizeof(double)));
    fscanf(fp,"%d",ReceiveData+i);
}
fclose(fp);

fp=fopen("DC.txt","r");
for(i=0;!feof(fp);i++){
//	ReceiveData=(double*)(ReceiveData ? realloc(ReceiveData,sizeof(double)*(i+1)):malloc(sizeof(double)));
    fscanf(fp,"%d",ReceiveData_L+i);
}
fclose(fp);
return 0;
}
*/

int FindPeak(double* in_data,int* in_peak,int * result,int cnt)
{
	
	double mean;
	int index_max,i;
	static double t;
	int s,d;
	mean=0;
	for(i=0;i<cnt;i++)
	{
		if(in_data[*(in_peak+i)]>mean)
		{			
			mean=in_data[*(in_peak+i)];
			index_max=i;
		}
    }
    
   for(i=0;i<index_max;i++)
   {
		if ( ((ks1*mean)<in_data[in_peak[i]]) && (in_data[in_peak[i]]<(ks2*mean)) )
			s=i;
   }

   for(i=index_max;i<cnt;i++)
   {
		if ( ((kd1*mean)<in_data[in_peak[i]]) && (in_data[in_peak[i]]<(kd2*mean)) )
			d=i; //没有找到合适的点，返回一个负数
   }
   result[0]=in_peak[s];//传递出去，对应到直流分量上去
   result[1]=in_peak[d];
   t=0;
   for(i=0;i<cnt-1;i++)
	{
		t+=in_peak[i+1]-in_peak[i];
	}
	return (int)(60.0/(t/(cnt-1)*(1/200.0)));
}
