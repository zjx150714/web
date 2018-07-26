#include<stdio.h>
#include<signal.h>
#include<strings.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#define MAX 1024
#define HOME_PAGE "index.html"//首页
static void usage(const char *proc)
{
	printf("Usage: %s port\n",proc);
}
int startup(int port)
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0){
		perror("socket");//错误消息打印到日志
		exit(2);
	}
	//保证服务器主动断开链接时，不能让服务器因timewait关系而立即重启
	int opt = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));



	//填充本地消息
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(port);

	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind");
		exit(3);
	}
	if(listen(sock,5)<0){
		perror("listen");
		exit(4);
	}
	return sock;
}
//按行获得文本
int get_line(int sock,char line[],int size)
{
	int c = 'a';
	int i = 0;
	ssize_t s = 0;
	while(i<size-1 && c!= '\n'){
		s = recv(sock,&c,1,0);
		if(s>0){
			if(c=='\r'){//特殊处理
				//\r->\n or \r\n->\n
				recv(sock,&c,1,MSG_PEEK);//? -> \r
					if(c != '\n'){
						c='\n';
					}
					else{
						recv(sock,&c,1,0);
					}
				}
			line[i++] = c;//\n
		}
		else{//错误处理
			break;
		}
	}
	line[i] = '\0';//给一行字符后面加上结束符'\0'
	return i;//返回一行字符的个数
}
//将剩下的代码读完
void clear_header(int sock)
{
	char line[MAX];
	do
	{
		get_line(sock,line,sizeof(line));
	}
	while(strcmp(line,"\n")!=0);
}

//非cgi方式，只是简单的网页显示
void echo_www(int sock,char* path,int size,int *err)
{
	//从sock里读数据一直读到空行停下来，解决TCP粘包问题
	clear_header(sock);

	//sendfile()函数  高效 在两个文件描述符之间拷贝数据
	int fd = open(path,O_RDONLY);
	if(fd<0)
	{
		*err=404;
		return;
	}
	//响应
	char line[MAX];
	sprintf(line,"HTTP/1.0 200 OK\r\n");
	//发送状态行
	send(sock,line,strlen(line),0);
	//响应类型
	sprintf(line,"Content-Type:text/html;charset=ISO-8859-1\r\n");
	send(sock,line,strlen(line),0);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);
	// 将path写到sock中
	 sendfile(sock,fd,NULL,size);
	 close(fd);
}
void echo_error(int code)
{
	switch(code)
	{
		case 404:
			break;
		case 501:
			break;
		default:
			break;
	}
}
int exe_cgi(int sock,char path[],char method[],char *query_string)
{
	//printf("AAAAAAAAAAA\n");
	char line[MAX];
	int content_length = -1;
    //三个缓冲区
	char method_env[MAX/32];//环境变量
	char query_string_env[MAX];
	char content_length_env[MAX/16];

	if(strcasecmp(method, "GET") == 0){
		clear_header(sock);
	}else{
		do{
			get_line(sock,line,sizeof(line));
			if(strncmp(line,"Content-Length: ",16) == 0){
				content_length = atoi(line+16);
			}
		}while(strcmp(line, "\n") !=0);
		if(content_length == -1){
			return 404;
		}
	}
	sprintf(line,"HTTP/1.0 200 OK\r\n");
	   //发送状态行
	   send(sock,line,strlen(line),0);
	   //响应类型
	   sprintf(line,"Content-Type:text/html;charset=ISO-8859-1\r\n"    );
	  send(sock,line,strlen(line),0);
	  sprintf(line,"\r\n");
	  send(sock,line,strlen(line),0);

	int input[2];
	int output[2];

	pipe(input);
	pipe(output);
	pid_t id = fork();
	if(id<0){
		return 404;
	}
	else if(id == 0){
		close(input[1]);
		close(output[0]);
		dup2(input[0],0);
		dup2(output[1],1);

		//导出环境变量
		 sprintf(method_env, "METHOD=%s",method);
		 //设置到环境变量列表里
		 putenv(method_env);
         
		 if(strcasecmp(method, "GET")==0){
		 sprintf(query_string_env, "QUERY_STRING=%s",query_string);
		 putenv(query_string_env);
		 }
		 else{
			 sprintf(content_length_env, "CONTENT_LENGTH=%d",content_length);
		 putenv(content_length_env);
		 }
		// printf("method: %s,path: %s\n",method,path);
         printf("method: %s");
		//method,GET[query_string],POST[content_length]
		execl(path,path,NULL);
		exit(1);

	}
	else{  //father
		close(input[0]);
		close(output[1]);

		char c;
		if(strcasecmp(method,"POST")==0){
			int i = 0;
			for(;i<content_length;i++){
				read(sock,&c,1);
				write(input[1],&c,1);//有效分离报头和正文，通过特殊符号+字描述字段描述有效字段长度，解决粘包问题（定长报头/字描述）
			}
		}
		//读取数据
		while(read(output[0],&c,1)>0){
			send(sock,&c,1,0);

		waitpid(id,NULL,0);
		close(input[1]);
		close(output[0]);
	}
		return 200;
	}
}

static void *handler_request(void *arg)
{
	int sock = (int)arg;
	char line[MAX];
	char method[MAX/32];//保存方法
	char url[MAX];
	char path[MAX];//资源路径
	int errCode=200;
	int cgi = 0;//通用网关接口,内置的一种标准
	char *query_string = NULL;//存放GET方法的参数
#ifdef Debug
	//打印信息，用于调试
	do{
		get_line(sock,line,sizeof(line));
		printf("%s",line);
	}while(strcmp(line,"\n")!=0);//说明把请求报头读完了
#else
	if(get_line(sock,line,sizeof(line))<0){//处理错误码
		errCode = 404;
		goto end;//到结尾，直接关闭链接
	}
	//走到这里说明第一行成功
	//get method
	//line[]=GET /index.php?a=100&b=200 HTTP/1.1
	int i = 0;
	int j = 0;
	while(i<sizeof(method)-1 && j < sizeof(line)&& !isspace(line[j])){
		method[i] = line[j];
		i++,j++;
	}
	method[i] = '\0';
	//判断方法要么是post方法要么是get方法
	//数据拼接到url后面，即get方法；post方法数据在正文部分，post方法一定带参数
	if(strcasecmp(method,"GET")==0){
	}
	else if(strcasecmp(method,"POST")==0){//strcasecmp是忽略大小写的字符串比较，返回值为真说明不相等
		cgi = 1;
	}
	else{
		errCode = 404;
		goto end;
	}

	while(j<sizeof(line) && isspace(line[j])){//一直是空行
		j++;
	}
	//提取url
	i=0;
	while(i<sizeof(url)-1 && j<sizeof(line) && !isspace(line[j])){
			url[i] = line[j];
			i++,j++;
	}
	url[i] = '\0';
	//url  ?左边是资源，给url；  右边是提交给资源的参数，给query_string
	if(strcasecmp(method,"GET")==0){
		query_string = url;
		while(*query_string){
			if(*query_string == '?'){
				*query_string = '\0';
				query_string++;
				cgi = 1;//get方法带参，以cgi方式运行
				break;
			}
			query_string++;
		}
	}
	//method[GET,POST],cgi[0|1],url[],query_String[NULL|arg]
	//url->wwwroot/a/b/c.html
	sprintf(path,"wwwroot%s",url);
	if(path[strlen(path)-1]=='/'){//url为/，直接拼接
		strcat(path,HOME_PAGE);
	}

    //printf("method:%s,path:%s\n",method,path);
	struct stat st;//确定文件是否存在，并且可以拿到属性信息
	if(stat(path,&st)<0){
		errCode = 404;
		goto end;
	}
	else{
		 if(S_ISDIR(st.st_mode)){
			 strcat(path,HOME_PAGE);
		 }else{
			 if((st.st_mode & S_IXUSR) || \
				(st.st_mode & S_IXGRP) ||\
				(st.st_mode & S_IXOTH)){
			cgi = 1;
		}
		}
		 //文件真实存在 
		 //如果以cgi方式运行
		 if(cgi){
			 errCode=exe_cgi(sock,path,method,query_string);
	}
		else{//非cgi情况，一定时GET方法
			printf("method: %s,path: %s\n",method,path);
			//method[GET,POST], cgi[0|1], url[],query_String[NULL|arg]
			echo_www(sock,path,st.st_size,&errCode);
		}
	}


#endif
 end:
	if(errCode != 200){
		echo_error(errCode);
	}
	close(sock);

}

int main(int argc,char *argv[])
{
	if(argc!=2){
		usage(argv[0]);
		return 1;
	}
	int listen_sock = startup(atoi(argv[1]));
	//事件处理
    signal(SIGPIPE,SIG_IGN);	
	for(; ;){
		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		int new_sock = accept(listen_sock,\
				(struct sockaddr*)&client,&len);
		if(new_sock < 0)
		{
			perror("accept");
			continue;//继续获取新链接
		}
		pthread_t id;
		pthread_create(&id,NULL,handler_request,(void*)new_sock);
		pthread_detach(id);
	}
}

