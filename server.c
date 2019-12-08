#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/sendfile.h>
#include<fcntl.h>
/*

#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/stat.h>

*/
char webpage[]=
"HTTP/1.1 200 OK\r\n"
"Content-type:text/html; charset:UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n";

unsigned long fsize(char* file){

    FILE * f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}

char* loadConfigFile(int line){
	FILE *pont_arq;
	char texto_str[20];
	pont_arq = fopen(".serverConfig", "r");

	int i;
	for (i = 0; i < line; i++) fgets(texto_str, 20, pont_arq);
	texto_str[strlen(texto_str)-1] = '\0';

	//fechando o arquivo
	fclose(pont_arq);

	char* retorno = (char*)malloc(20*sizeof(char));
	strcpy(retorno, texto_str);

	return retorno;
}

void printLog(char* buf){
	if(!strncmp(buf, "GET", 3)){
		printf("GET\n");
	}

	if(!strncmp(buf, "POST", 4)){
		printf("POST\n");
	}
	
	//printf("%s\n", buf);
}

void send_new(int fd, char *msg) {
 int len = strlen(msg);
 if (send(fd, msg, len, 0) == -1) {
  printf("Error ao Enviar\n");
 }
}

int main(int argc, char *argv[]){

	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len=sizeof(client_addr);
	int fd_server,fd_client;
	char buf[2048];
	/* Diretorio Raiz */
	char* rootDir = loadConfigFile(2);
	char* rootFile = loadConfigFile(5);

	printf("ROOT DIR:  [%s] -- \n", rootDir);
	printf("ROOT FILE: [%s] -- \n", rootFile);
	printf("PORT :	   [%s] -- \n", loadConfigFile(8));

	int on=1,fdimg,fdfile;

	/* COMEÇO DA INTERFACE DE UM SOCKET */
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_server < 0){
		perror("socket");
		exit(1);
	}

	/* CONFIGURANDO AS OPÇÕES */
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi( loadConfigFile(8) ));

	/* ATRIBUINDO UM "NOME" */
	if(bind(fd_server, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
		perror("bind");
		close(fd_server);
		exit(1);
	}

	/* NUMERO MAXIMO DE CONEXÕES*/
	if(listen(fd_server, 10) == -1){
		perror("listen");
		close(fd_server);
		exit(1);
	}

	printf("Servidor Iniciado:\n");

	while(1){
			/* NOVO "CLIENTE" */
			fd_client = accept(fd_server, (struct sockaddr*)&client_addr, &sin_len);		
			if(fd_client == -1){
				perror("Conexão Falhou... \n");
				continue;
			}
			//printf("----------------------------------+\nObtendo Conexão para o Cliente.   |\n----------------------------------+\n");
			
			if(!fork()){
				/* PROCESSO FILHO */
				close(fd_server);		
				/* LIMPAR BUFFER */
				memset(buf, 0, 2048);
				read(fd_client, buf, 2047);

				/* GET VERBOSE */
				//printf("%s\n", buf);
				printLog(buf);
				
				/* VERIFICA O TIPO DE ARQUIVO e o Nome do Arquivo*/
				char buf_filename[2048];
				char buf_type[2048];
				memcpy(buf_filename, buf, 2048);
				memcpy(buf_type, buf, 2048);
				
				char *filename;
				char *type;
				filename = strtok(buf_filename, " ");
				filename = strtok(NULL, " ");
				filename = strtok(filename, "/");

				type = strtok(buf_type, ".");
				type = strtok(NULL, ".");
				type = strtok(type, " ");

				char bufConcat[256];
				snprintf(bufConcat, sizeof bufConcat, "%s%s", rootDir, filename);

				/* ENVIA PARA O CLIENTE VISUALIZAR O FORMATO */
				if(!strcmp(type, "ico") || !strcmp(type, "jpg") || !strcmp(type, "jpeg") || !strcmp(type, "gif")){	
					
					/* DEBUG */
					printf("	FileName: [%s]\n", filename);
					printf("	Type: [%s]\n", type);

					char header_img[]=
						"HTTP/1.1 200 OK\r\n"
						"Content-type:image; charset:UTF-8\r\n\r\n";
					send_new(fd_client, header_img);

					//send(fd_client,header_img,sizeof(header_img),on);
					fdimg = open(bufConcat, O_RDONLY);
					sendfile(fd_client, fdimg, NULL, fsize(bufConcat));
					close(fdimg);
				}
				else if(filename == NULL){

					/* ENVIA PARA O CLIENTE VISUALIZAR A PAGINA PADRÃO */
					send_new(fd_client, webpage);
					snprintf(bufConcat, sizeof bufConcat, "%s%s", rootDir, rootFile);
					fdfile=open(bufConcat,O_RDONLY);
					sendfile(fd_client,fdfile,NULL,300);
					close(fdfile);
							
				}else{
					printf("	Acess: [%s]\n", filename);

					send_new(fd_client, webpage);
					fdfile=open(bufConcat,O_RDONLY);

					if(fdfile == -1){
						printf("404 File not found Error\n");
						send_new(fd_client, "HTTP/1.1 404 Not Found\r\n");
						send_new(fd_client, "Server : Web Server in C\r\n\r\n");
						send_new(fd_client, "<html><head><title>404 Not Found</head></title>");
					}else{
						printf("\n\n   FD FILE: %d\n\n\n",fdfile);
						sendfile(fd_client,fdfile,NULL,300);
					}
					close(fdfile);	
				}
				close(fd_client);
				//printf("\nConexão Encerrada!\n---------------------------------->\n\n");
				exit(0);
			}
			
			/* PROCESSO PAI */
			close(fd_client);
	}

	return 0;
}
