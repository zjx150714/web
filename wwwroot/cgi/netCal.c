#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#define MAX 1024
void mycal(char *buff)
{
	int x,y;
	sscanf(buff,"firstdata=%d&lastdata=%d",&x,&y);

	//输出网页
	printf("<html>\n");
	printf("<body>\n");
	printf("<h3>%d + %d = %d</h3>\n",x,y,x+y);
	printf("<h3>%d - %d = %d</h3>\n",x,y,x-y);
	printf("<h3>%d * %d = %d</h3>\n",x,y,x*y);
	if(y==0){
		printf("<h3>%d / %d = %d</h3>,%s\n",x,y,-1,"(zero)");
		 printf("<h3>%d %% %d = %d</h3>,%s\n",x,y,-1,"(zer0)");
	}
	else{

	printf("<h3>%d / %d = %d</h3>\n",x,y,x/y);
	printf("<h3>%d %% %d = %d</h3>\n",x,y,x%y);
	}
	printf("</body>\n");
	printf("</html>\n");
}
int main()
{
	char buff[MAX]={0};
	if(getenv("METHOD")){
		if(strcasecmp(getenv("METHOD"),"GET")==0){
			strcpy(buff,getenv("QUERY_STRING"));
		}else{
			int content_length = atoi(getenv("CONTENT_LENGHTH"));
			int i = 0;
			char c;
			for(;i<content_length;i++){
				read(0,&c,1);
				buff[i] = c;
			}
			buff[i] = '\0';
		}
	}
	printf("%s\n",buff);

   //	printf("echo Server : %s\n",getenv("METHOD"));
   //	printf("echo Server arg: %s\n",getenv("QUERY_STRING"));
   //	printf("hello cgi!\n");
    mycal(buff);
	return 0;
}
