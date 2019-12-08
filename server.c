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
typedef struct {
 char *ext;
 char *mediatype;
} extn;

//Possiveis Tipos de Media
extn extensions[] ={
 {"gif", "image/gif" },
 {"txt", "text/plain" },
 {"jpg", "image/jpg" },
 {"jpeg","image/jpeg"},
 {"png", "image/png" },
 {"ico", "image/ico" },
 {"zip", "image/zip" },
 {"gz",  "image/gz"  },
 {"tar", "image/tar" },
 {"htm", "text/html" },
 {"html","text/html" },
 {"php", "text/html" },
 {"pdf","application/pdf"},
 {"zip","application/octet-stream"},
 {"rar","application/octet-stream"},
 {0,0} };

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
		printf("\x1b[32m + [GET]\x1b[0m\n");
	}

	if(!strncmp(buf, "POST", 4)){
		printf("\x1b[32m + [POST]\x1b[0m\n");
		printf("%s\n", buf);
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

	system("@cls||clear");
	printf("\nROOT DIR:  [%s] \n", rootDir);
	printf("ROOT FILE: [%s] \n", rootFile);
    printf(
            "Servidor Iniciado: %shttp://127.0.0.1:%s%s\n",
            "\033[92m",loadConfigFile(8),"\033[0m\n"
    		);

	while(1){
			/* NOVO "CLIENTE" */
			fd_client = accept(fd_server, (struct sockaddr*)&client_addr, &sin_len);		
			if(fd_client == -1){
				perror("Conexão Falhou... \n");
				continue;
			}
			if(!fork()){
				/* PROCESSO FILHO */
				close(fd_server);		
				/* LIMPAR BUFFER */
				memset(buf, 0, 2048);
				read(fd_client, buf, 2047);

				/* GET VERBOSE */
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
				//filename = strtok(filename, "/");

				type = strtok(buf_type, ".");
				type = strtok(NULL, ".");
				type = strtok(type, " ");

				char bufConcat[256];
				snprintf(bufConcat, sizeof bufConcat, "%s%s", rootDir, filename);
				fdfile = open(bufConcat, O_RDONLY);
                
				/* ENVIA PARA O CLIENTE VISUALIZAR O FORMATO */
				int i;
   				for (i = 0; extensions[i].ext != NULL; i++) {
					if (!strcmp(type, extensions[i].ext)) {
						send_new(fd_client, "HTTP/1.1 200 OK\r\n");
						send_new(fd_client, "Content-type:image; charset:UTF-8\r\n\r\n");
						sendfile(fd_client, fdfile, NULL, fsize(bufConcat));
						close(fd_client);
					}
				}

				send_new(fd_client, webpage);
	
				if(!strcmp("/", filename)){
					printf("AAAAAAAAAA");
					/* ENVIA PARA O CLIENTE VISUALIZAR A PAGINA PADRÃO */
					snprintf(bufConcat, sizeof bufConcat, "%s/%s", rootDir, rootFile);
					printf("BBBBBBBB: %s\n\n", bufConcat);
					fdfile = open(bufConcat, O_RDONLY);

					sendfile(fd_client,fdfile,NULL,fsize(bufConcat));
				}else{
					printf("	Acess: [%s]\n", bufConcat);
					printf("	Acess: [%s]\n", type);
					/* Tratamento ERROR 404*/
					if(fdfile == -1){
						printf("404 File not found Error\n");
						send_new(fd_client, "HTTP/1.1 404 Not Found\r\n");
						send_new(fd_client, "<html><head><title>404 Not Found</head></title>");
					}else{
						sendfile(fd_client,fdfile,NULL,fsize(bufConcat));
					}
				}
                
				close(fdfile);	
				close(fd_client);
				exit(0);
			}
			
			/* PROCESSO PAI */
			close(fd_client);
	}
	

	return 0;
}
