#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "defs.h"

#define PORT 58000
#define SIZE 100
#define BUFFERSIZE 1000
#define max(A,B) ((A)>=(B)?(A):(B))

int comunicateSend(int fd, char buffer[], char msg[], struct sockaddr_in serveraddr, int addrlen);
void reg(char *info, char *msg, char *surname, FILE *ulist);
void list(FILE *ulist);
RQST rqstType(char *rqst);
FILE* unreg(char *name, char *msg, FILE *ulist,char *nome_file);
int searchUser(FILE *ulist, char *cmp);
char* returnInfo(FILE *ulist,int linha);

int main(int argc, char *argv[])
{
	int fd, sfd,cfd, port,port_aux, usr_in, n, addrlen, addrlen2, counter, terminate, a, auxLinha, count_scan=0;
	struct hostent *hostptr;
	struct in_addr *aux_struct;
	struct sockaddr_in serveraddr, serveraddr2, serveraddr3;
	char *apelido, nome_file[SIZE], *ip, *instr, *auxInfo, *auxstr = (char*) malloc(sizeof(char)*100), *auxstr0 = (char*) malloc(sizeof(char)*100);
	char *auxstr1 = (char*) malloc(sizeof(char)*100), *request = (char*) malloc(sizeof(char)*100), *saux = (char*) malloc(sizeof(char)*100);
	char buffer[BUFFERSIZE], msg[1000], cmd[10], *ip_aux = (char*) malloc(sizeof(char)*100);
	fd_set skt;
	struct timeval tv;
	FILE *ulist;
	
	/*arguments verification*/							
	if( argc < 7 || argc > 9) 
    {
		printf("Comando não aceite. Argumentos não válidos");
        exit(0);
    }
    
    /*saves arguments*/
    port = atoi(argv[6]);
    apelido = (char*)malloc(strlen(argv[2])*sizeof(char)); 
    strcpy(apelido,argv[2]);        
	ip = (char*)malloc(strlen(argv[4])*sizeof(char));
    strcpy(ip,argv[4]); 
     
    /*socket opening and verification*/  
	fd = socket(AF_INET, SOCK_DGRAM,0);
	if(fd == -1)exit(-1);
	sfd = socket(AF_INET, SOCK_DGRAM,0);
	if(sfd == -1)exit(-1);
	cfd = socket(AF_INET, SOCK_DGRAM,0);
	if(cfd == -1)exit(-1);

	if((hostptr = gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL) exit(1);
	
	/*init UDP client to comunicate with SA*/
	memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);
	addrlen = sizeof(serveraddr);		

	/*Register in main server*/
	sprintf(msg,"SREG %s;%s;%d", apelido, ip, port);
	if(comunicateSend(fd, buffer, msg, serveraddr, addrlen)==1){
		/*Timeout case*/
		printf("NAO FOI POSSIVEL REGISTAR NO SA, REINICAR PROGRAMA!\n");		
	}else{	
		printf("%s\n",buffer);
	}
	
	/*init UDP server snp*/
	memset((void*)&serveraddr2,(int)'\0', sizeof(serveraddr2));
	serveraddr2.sin_family = AF_INET;
	serveraddr2.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr2.sin_port = htons((u_short)port);	
	bind(sfd, (struct sockaddr*)&serveraddr2, sizeof(serveraddr));
	addrlen2 = sizeof(serveraddr2);

	/*init snp client*/
	memset((void*)&serveraddr3,(int)'\0', sizeof(serveraddr3));
	serveraddr3.sin_family = AF_INET;
	aux_struct = (struct in_addr*)malloc((sizeof(struct in_addr)));
	
	/*stdin socket*/
	usr_in = fileno(stdin);
	
	/*create user file*/
	sprintf(nome_file,"%s",apelido);
	strcat(nome_file,".txt");
	ulist = fopen(nome_file, "w+");
	
	terminate = 0;
	while(terminate == 0){
		
		/*clear set and init file descriptors*/
		FD_ZERO(&skt);
		FD_SET(sfd, &skt);
		FD_SET(usr_in, &skt);
		
		/*set timeout interval*/
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		/*clean keyboard buffer*/
		fflush(stdout);
		
		/*set select function*/
		counter = select(max(usr_in,sfd)+1,&skt,NULL,NULL,&tv);
		if(counter < 0)
			exit(0);
		if(counter >= 1){
			
			if(FD_ISSET(usr_in, &skt)){
				scanf("%s",cmd);
				if(strcmp(cmd,"exit") == 0){
					
					/*Unregister from main server*/
					sprintf(msg,"SUNR %s", apelido);
					if(comunicateSend(fd, buffer, msg, serveraddr, addrlen)!=1){	
					printf("%s - %s unregistered\n", buffer, apelido);
					terminate = 1;
					}
				}else if(strcmp(cmd,"list") == 0){
					list(ulist);
					rewind(ulist);	
				}else
					printf("Invalid command\n");
			}
			if(FD_ISSET(sfd,&skt)){
				
				/*get received message*/
				n = recvfrom(sfd,buffer,sizeof(buffer),0,(struct sockaddr*)&serveraddr2,&addrlen);
				buffer[n] = '\0';
				/*get instruction from buffer*/
				sscanf(buffer, "%s %s", request, saux);
				
				switch(rqstType(request)){
					case REG: /*Register client in SNP*/
							reg(saux, msg, apelido, ulist);
							rewind(ulist);
							sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);
							break;
					case UNR: /*Unregister client from SNP*/
							ulist = unreg(saux, msg, ulist,nome_file);
							rewind(ulist);
							sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);
							break;
					case QRY: /*Look for client*/
							sscanf(saux, "%*[^.].%s", auxstr0);
							if(strstr(auxstr0,apelido) != NULL){
								/*Case which the pretended client surname is the same that this SNP*/
								auxLinha = searchUser(ulist,saux);
								rewind(ulist);
								if(auxLinha != -1){
									/*Client found, returns the client info*/
									auxInfo = returnInfo(ulist, auxLinha);
									rewind(ulist);
									sprintf(msg,"RPL %s",auxInfo);
									sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);
								}else{
									/*Client not found*/
									sprintf(msg,"RPL");
									sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);	
								}
							}else{
								strcpy(auxstr0, saux); 
								sscanf(auxstr0, "%*[^.].%s", saux);

								sprintf(msg, "SQRY %s", saux);
								/*Questions the SA about the existence of SNP with the same surname as the pretended client*/
								if(comunicateSend(fd, buffer, msg, serveraddr, addrlen)!=1){
									count_scan = sscanf(buffer, "%s %s", auxstr, auxstr1);
									/*checks if SA found snp*/
									if(count_scan != 2){
										/*snp not found*/
										sprintf(msg, "RPL");
										sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);
									}
									/*snp found*/
									else if(rqstType(auxstr) == SRPL){	
										/*saves the received snp info */
										sscanf(auxstr1,"%*[^;];%[^;];%d", ip_aux, &port_aux);
									
										/*inserts new snp ip and port in socket struct to comunicate with another snp*/
										inet_aton(ip_aux, aux_struct);
										serveraddr3.sin_addr.s_addr = aux_struct->s_addr;
										serveraddr3.sin_port = htons((u_short)port_aux);
										addrlen = sizeof(serveraddr3);
									
										sprintf(msg, "QRY %s", auxstr0);
										if(comunicateSend(cfd, buffer, msg, serveraddr3, addrlen)!=1){
											count_scan = sscanf(buffer, "%s %s", auxstr, auxstr1);
									
											/*Checks answer from the other snp*/
											if(rqstType(auxstr) == RPL){
												if(count_scan == 1)
												/*Client not found*/
												sprintf(msg, "RPL");
												else if (count_scan ==2)
												/*CLient found, returns the client info*/
												sprintf(msg, "RPL %s",auxstr1);
											}
										sendto(sfd,msg,strlen(msg)+1,0,(struct sockaddr*)&serveraddr2,addrlen);
										}	
									}
								}
							  }
							break;
					default: 
							break;
				}		
			}
		}
	}
	
	printf("EXIT\n");
	/*Frees memory, closes sockets and files used*/
	free(aux_struct);
	free(apelido);
	free(ip);
	free(ip_aux);
	free(auxstr);
	free(auxstr0);
	free(auxstr1);
	free(request);
	free(saux);
	close(fd);
	close(sfd);
	close(cfd);
	fclose(ulist);
}

/*Return the number of the line in the data base file, where is saved the pretended client*/

int searchUser(FILE *ulist, char *cmp){
	
		char buffer[SIZE], *name = NULL;
		int n = 0;
		
		while(fgets(buffer, SIZE, ulist) != NULL){
			n++;
			if(strstr(buffer, cmp) != NULL)		
				return n;
		}
		return -1;
}

/*Return the name.surname;ip;port from the pretended line in the data base file*/

char* returnInfo(FILE *ulist,int linha){
		
		char *msg = (char*) malloc(sizeof(char)*SIZE);
		char buffer[SIZE];
		int i;	
		for(i = 0;i!=linha;i++){
			fgets(buffer,SIZE,ulist);
			sscanf(buffer, "%[^\n]s", msg);
		}
		return msg;
}

/*Return the Protocol identifier message from a the string received by the snp*/

RQST rqstType(char *rqst){
	if(strcmp(rqst, "REG") == 0)
		return REG;
	else if(strcmp(rqst, "UNR") == 0)
		return UNR;
	else if(strcmp(rqst, "QRY") == 0)
		return QRY;
	else if(strcmp(rqst, "SQRY") == 0)
		return SQRY;
	else if(strcmp(rqst, "SRPL") == 0)
		return SRPL;
	else if(strcmp(rqst, "RPL") == 0)
		return RPL;
	else if(strcmp(rqst, "NAME") == 0)
		return NAME;
	else if(strcmp(rqst, "AUTH") == 0)
		return AUTH;						
	else
		printf("Failed to process schat request!\n");
}

/*Regist the pretended client by saving him in the data base file*/

void reg(char *info, char *msg, char *surname, FILE *ulist){
	
	char *info_surname = (char*) malloc(sizeof(char)*100);
	char *name = (char*) malloc(sizeof(char)*100);
	char *ip = (char*) malloc(sizeof(char)*100);
	int port;
	sscanf(info,"%[^;];%[^;];%d", name, ip, &port); //get info
	sscanf(name,"%*[^.].%s", info_surname); //get surname

	if(searchUser(ulist, name) == -1 && strcmp(surname, info_surname) == 0){
		fprintf(ulist, "%s;%s;%d\n", name, ip, port);
		sprintf(msg, "OK");
	} else 
		sprintf(msg, "NOK");
	
	free(info_surname);
	free(name);
	free(ip);
}

/*Unregist the pretended client from the server by deleting him from data base in file*/

FILE* unreg(char *name, char *msg, FILE *ulist,char *nome_file){
	
	char aux[SIZE];
	FILE *fp = fopen("temp.txt", "a+");
	int n, line = 0;
	
	n = searchUser(ulist, name);
	rewind(ulist);
	if(n == -1){
		sprintf(msg, "NOK User not registered!");
		return ulist;
	}else {
		while(fgets(aux, SIZE, ulist) != NULL){
			
			line++;
			if(line != n)
				fprintf(fp, aux);
		}
		sprintf(msg, "OK");
	}
	
	fclose(ulist);
	rename("temp.txt",nome_file);	
	return fp;
}

/*Prints in terminal all clients registed in server*/

void list(FILE *ulist){
	
	char aux[SIZE];
	if(fgets(aux, SIZE, ulist) == NULL){
		printf("No users registered \n");
		return NULL;
	}
	rewind(ulist);	
	while(fgets(aux, SIZE, ulist) != NULL){
		printf("%s\n",aux);	
	}	
}		

/*Sends a message and receives an answer through the fd socket, if the answer takes more than 5 seconds it
timesout returning 1 (UDP connection)*/

int comunicateSend(int fd, char buffer[], char msg[], struct sockaddr_in serveraddr, int addrlen){
	int n, count;
	fd_set skt;
	struct timeval tv;
	
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&skt);
	FD_SET(fd, &skt);
	
	sendto(fd,msg,strlen(msg),0,(struct sockaddr*)&serveraddr,addrlen);
	if(select(fd+1, &skt,NULL,NULL, &tv) == 0){
		printf("UDP connection timeout!\n");
		FD_CLR(fd, &skt);
		return 1;
	}	
	n = recvfrom(fd,buffer,BUFFERSIZE,0,(struct sockaddr*)&serveraddr,&addrlen);
	buffer[n] = '\0';
	return 0;
}
