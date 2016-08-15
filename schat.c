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

#define SIZE 100
#define BUFFERSIZE 1000
#define AUXSIZE 20
#define max(A,B) ((A)>=(B)?(A):(B))

RQST rqstType(char *rqst);
int searchAuth(FILE *fp, int cmp);
 
int main(int argc, char *argv[])
{

	int fd, sfd, newfd, cfd, afd, fdcon, n, scport, snport, port_aux, a, addrlen, terminate = 0, usr_in, counter, maxfds, join_flag=0;
	int cfd_close = 0, auth_flag = 0, line_rand,line_rand_2;
	char *np, *ip, np_find[SIZE], *snip, *aux_str = (char*) malloc(sizeof(char)*SIZE), *aux_str1 = (char*) malloc(sizeof(char)*SIZE);
	char *saux, msg[BUFFERSIZE], buffer[BUFFERSIZE], *ip_aux = (char*) malloc(sizeof(char)*SIZE), *connected_usr = (char*) malloc(sizeof(char)*SIZE);
	char *auth_name = (char*) malloc(sizeof(char)*SIZE);
	struct hostent *hostptr;
	struct sockaddr_in clientaddr, clientaddr2, serveraddr;
	struct in_addr *aux_struct;
	struct timeval tv;
	fd_set skt;
	char *cmd = (char*) malloc(sizeof(char)*AUXSIZE); 
	enum{idle,busy} state;
	time_t t;
	unsigned char char_rand;
	int int_rnd, int_rnd2, int_rnd3, int_rnd4;
	FILE *fp;
	srand((unsigned) time(&t));
	aux_struct = (struct in_addr*)malloc((sizeof(struct in_addr)));
	
	/*gather initial arguments data*/
	np = (char*)malloc(SIZE*sizeof(char));
	strcpy(np, argv[2]);
	ip = (char*)malloc(sizeof(argv[4])*sizeof(char));
	strcpy(ip, argv[4]);
	snip = (char*)malloc(sizeof(argv[8])*sizeof(char));
	strcpy(snip, argv[8]);
	scport = atoi(argv[6]);
	snport = atoi(argv[10]);
	
	/*init sockets*/
	fd = socket(AF_INET, SOCK_DGRAM,0);
	if(fd == -1){ printf("fd down\n"); exit(-1);}
	sfd = socket(AF_INET, SOCK_STREAM,0);
	if(sfd == -1){ printf("sfd down\n");exit(-1);}
	cfd = socket(AF_INET, SOCK_STREAM,0);
	if(cfd == -1){ printf("cfd down\n");exit(-1);}
	
	
	
	if((hostptr = gethostbyname(snip)) == NULL){printf("gethostbyname down\n"); exit(0);}
	
	/*init client snp*/
	memset((void*)&clientaddr,(int)'\0', sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
	clientaddr.sin_port = htons((u_short)snport);
	addrlen = sizeof(clientaddr);
	
	/*init TCP server schat*/
	memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)scport);
	if(bind(sfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){ printf("bind down\n");exit(-1);}
	listen(sfd, 5);
	
	/*init TCP client schat*/
	memset((void*)&clientaddr2,(int)'\0', sizeof(clientaddr2));
	clientaddr2.sin_family = AF_INET;
	
	/*stdin socket*/
	usr_in = fileno(stdin);
	state = idle;

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
		
		if(state == busy){
			FD_SET(cfd, &skt);
			maxfds = max(maxfds, cfd);
		}
		
		/*set select function*/
		counter = select(max(maxfds,sfd)+1,&skt,NULL,NULL,&tv);
		if(counter < 0){
			printf("Select error\n");
			exit(0);
		}
		if(counter >= 1){
			if(FD_ISSET(usr_in, &skt)){
				scanf("%[^\n]s",cmd);
				getchar();
				if(strcmp(cmd,"join") == 0 && join_flag == 0){
					/*Register in SNP*/
					sprintf(msg, "REG %s;%s;%d", np, ip,scport);
					if(comunicate(fd, buffer, msg, clientaddr, addrlen)!=1){	
						printf("join: %s\n", buffer);	
						if(strcmp(buffer, "NOK") == 0){
							printf("Name already registered or invalid surname for this SNP!\n");
							terminate = 1;
							continue;
						}
						join_flag = 1;
					}		
				}
				else if(strcmp(cmd,"exit") == 0){
						terminate = 1;
				}
				else if(join_flag == 1){
					if(strcmp(cmd,"leave") == 0){
						/*Unregister from SNP*/
						if(state != busy){
							sprintf(msg, "UNR %s", np);
							if(comunicate(fd, buffer, msg, clientaddr, addrlen)!=1){
								printf("leave: %s\n", buffer);
								join_flag = 0;
							}
						}else
							printf("Disconnect from conversation first! (disconnect)\n");
					}
					else if(strstr(cmd,"find ") != NULL){
						/*Ask SNP about the location of certain user*/
						sscanf(cmd,"%*[^ ]%s",aux_str);
						if(strstr(aux_str,".") == NULL){
							printf("Please write a valid username (name.surname)\n");
							continue;
						}
						sprintf(msg,"QRY %s",aux_str);
						if(comunicate(fd, buffer, msg, clientaddr, addrlen)!=1){
						printf("find: %s\n", buffer);
						}
					}
					else if(strstr(cmd, "connect ") != NULL){
						/*tries to establish a tcp connection with another user*/
						
						if(state != busy){
							sscanf(cmd,"%*[^ ]%s %s",aux_str, aux_str1);
							if(strstr(aux_str,".") == NULL){
								printf("Please write a valid username (name.surname)\n");
								continue;
							}
							if(strcmp(aux_str,np)!=0){
							
								/*Open keyfile*/
								sprintf(auth_name, "%s.txt", aux_str1);
								if((fp = fopen(auth_name, "r")) == NULL){
									printf("Error opening authentication file!\n");
									continue;
								}	
								/*first get the user, which we want to speak, location using the command "find"*/
								sprintf(msg,"QRY %s",aux_str);
								if(comunicate(fd, buffer, msg, clientaddr, addrlen)!=1){
									printf("connect: %s\n", buffer);
							
									/*save user info to establish connection*/
									if(sscanf(buffer,"RPL %[^;];%[^;];%d",aux_str, ip_aux,&port_aux) == 3){
										if(cfd_close == 1)
										if((cfd = socket(AF_INET, SOCK_STREAM,0)) == -1){
											printf("cfd down\n"); exit(0);}
										else cfd_close = 0;
										/*insert new schat ip and port in socket struct*/
										inet_aton(ip_aux, aux_struct);
										clientaddr2.sin_addr.s_addr = aux_struct->s_addr;
										clientaddr2.sin_port = htons((u_short)port_aux);
										connect(cfd, (struct sockaddr*)&clientaddr2, sizeof(clientaddr2));
							
										/*send a message to the connected user with his NAME introducing himself*/
										auth_flag = 0;
										sprintf(msg,"NAME %s\n", np);
										write(cfd, msg, sizeof(msg));
										if((n = read(cfd, buffer, sizeof(buffer))) != 0){
											if(n == -1){ printf("read n = -1\n"); exit(-1);}
											/*Read the first answer from the recipient client, which starts the authentication process*/
											sscanf(buffer,"AUTH %c", &char_rand);
											int_rnd = char_rand;
											int_rnd = searchAuth(fp, int_rnd);
											char_rand = int_rnd;
											sprintf(msg,"AUTH %c",char_rand);
											write(cfd, msg, sizeof(msg));
											auth_flag = 1;
										} 
										connected_usr = aux_str;
										state = busy;
									}else
										printf("User not found! Test with (find)\n");
								}
							}else
								printf("Cannot connect to yourself\n");
						}else
							printf("Connection already in progress!\n");
					}
					else if(strstr(cmd, "message ") != NULL){
						/*Send a message to the connected schat*/
						if(state == busy){
							sscanf(cmd, "message %[^\n]s",msg);
							write(cfd, msg, sizeof(msg));
						}else
							printf("Establish a connection with a user first! (connect name.surmane)\n");
					}
					else if(strcmp(cmd,"disconnect") == 0){
						/*Close TCP connection*/
						sprintf(msg, "disconnected from the conversation!");
						if(state == busy){
							auth_flag = 0;
							write(cfd, msg, sizeof(msg));
							close(cfd);
							cfd_close = 1;
							state = idle;
						} else 
							printf("Not connected to any user! (connect name.surname)\n");
					}
					else
						printf("Invalid command\n");
				}else
					printf("Registar no diretorio primeiro,(join)\n");
			}
			else if(FD_ISSET(sfd,&skt)){
				newfd = accept(sfd, (struct sockaddr*)&serveraddr,&addrlen);
				switch(state)
				{
					case idle: /*Accept a TCP connection from another user*/
							   cfd = newfd; 
							   if((n = read(cfd, buffer,sizeof(buffer))) != 0){
									if(n == -1){ printf("read n = -1\n"); exit(-1);}
									auth_flag = 0;
									/*Receives the first message from the user*/
									sscanf(buffer,"NAME %s\n", connected_usr);
									/*Open keyfile*/
									if((fp = fopen("auth.txt", "r")) == NULL){
										printf("Error opening authentication file!\n");
										close(cfd);
										cfd_close = 1;
										continue;
									}
									int_rnd = rand() % 256;
									line_rand = searchAuth(fp, int_rnd);
									char_rand = int_rnd;
									sprintf(msg,"AUTH %c",char_rand);
									/*Answer with an authentication challenge*/
									write(cfd, msg, sizeof(msg));
							   } 
							   state = busy; 
							   printf("Connection received from %s\n", connected_usr);
						break;
					case busy: /*Dont accept because is already in a TCP connection*/
							   if((n = read(newfd, buffer,sizeof(buffer))) != 0){
									if(n == -1){ printf("read n = -1\n"); exit(-1);}
									sscanf(buffer, "NAME %s\n", aux_str1);
									printf("%s tried to contact you!\n", aux_str1);
							   } 
							   sprintf(msg, "User is already in a connection! Try again later");
							   write(newfd, msg, sizeof(msg));
							   close(newfd);
						break;
					default: 
						break;	
				}	
				
			}
			else if(FD_ISSET(cfd,&skt)){
				if((n = read(cfd, buffer,sizeof(buffer))) != 0){
					if(n == -1){ printf("read n = -1\n"); exit(-1);}
					/*Receives the answer of the first authentication question*/
					if(auth_flag == 0){
						sscanf(buffer, "AUTH %c", &char_rand);
						int_rnd = char_rand;
						if(int_rnd == line_rand){
							/*First authentication complete, now the other user must do his authentication*/
							auth_flag = 2;
							printf("First authentication complete\n");
							sprintf(msg,"Name %s\n",np);
							/*message the other user telling him to start his authentication */
							write(cfd, msg, sizeof(msg));
						}else{
							/*Error in first authentication, close tcp connection, connection not secure*/
							printf("ERROR in first authtentication\n");
							close(cfd);
							cfd_close = 1;
							fclose(fp);
							state = idle;
							auth_flag = 0;
						}	
					}
					else if(auth_flag == 1){
						/*sends the second authentication challenge*/
						auth_flag = 3;
						int_rnd = rand() % 256;
						line_rand = searchAuth(fp, int_rnd);
						char_rand = int_rnd;
						sprintf(msg,"AUTH %c",char_rand);
						write(cfd, msg, sizeof(msg));
					}
					else if(auth_flag == 2){
						/*Receive and answer to the second authentication challenge*/
						sscanf(buffer,"AUTH %c", &char_rand);
						int_rnd = char_rand;
						int_rnd = searchAuth(fp, int_rnd);
						char_rand = int_rnd;
						sprintf(msg,"AUTH %c",char_rand);
						write(cfd, msg, sizeof(msg));
						auth_flag = 4;
						fclose(fp);
					}
					else if(auth_flag == 3){
						/*Reiceve the answer to the second authentication, checks if is correct or wrong*/
						sscanf(buffer, "AUTH %c", &char_rand);
						int_rnd = char_rand;
						if(int_rnd == line_rand){
							printf("Second authentication complete\n");
							printf("Full authentication complete!\n");
							sprintf(msg,"Full authentication complete!");
							write(cfd, msg, sizeof(msg));
							auth_flag = 4;
						}else{
							printf("ERROR in second authtentication\n");
							close(cfd);
							cfd_close = 1;
							state = idle;
							auth_flag = 0;
						}
						fclose(fp);
					}
					/*Both authentications successful, now users can comunicate at will*/
					else if(auth_flag == 4){
						/*Print the message received from another user*/
						printf("%s >> %s\n",connected_usr, buffer);
					}							
				}
				else{
					close(cfd);
					auth_flag = 0;
					cfd_close = 1;
					state = idle;
				}
			}
		}		
	}
	printf("EXIT\n");

	/*clean memory,close sockets and files*/
	//fclose(fp);
	free(np);
	free(ip);
	free(snip);
	free(aux_struct);
	free(cmd);
	free(auth_name);
	free(aux_str);
	free(aux_str1);
	free(connected_usr);
	free(ip_aux);
	if(cfd_close == 0)
		close(cfd);
	close(sfd);
	close(fd);
}

/*Search the keyfile for the "cmp" number and returns the number of the line where he is located*/

int searchAuth(FILE *fp, int cmp){
	
		char buffer[SIZE];
		int n;
		
		for(n = 0; n != cmp; n++)
			fgets(buffer, SIZE, fp);
					
		sscanf(buffer, "%d", &cmp);
		rewind(fp);
		return cmp;
}

/*Sends a message and receives an answer through the fd socket, if the answer takes more than 5 seconds it
timesout returning 1 (UDP connection)*/

int comunicate(int fd, char buffer[], char msg[], struct sockaddr_in serveraddr, int addrlen){
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
 
/*Returns the Protocol identifier message from a the string received by the snp*/

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
	else if(strcmp(rqst, "message") == 0)
		return MSG;							
	else
		printf("Failed to process schat request!\n");
}	
