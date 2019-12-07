#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/sendfile.h>
#include<fcntl.h>
//#include<sys/types.h>
//#include<netinet/in.h>
//#include<netdb.h>
//#include<sys/stat.h>

char webpage[]=
"HTTP/1.1 200 OK\r\n"
"Content-type:text/html; charset:UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n";


unsigned long fsize(char* file)
{
    FILE * f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}

int main(int argc, char *argv[]){

	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len=sizeof(client_addr);
	int fd_server,fd_client;
	char buf[2048];
	/* Diretorio Raiz */
	char rootDir[] = "./www/";
	char rootFile[] = "index.html";

	int on=1,fdimg,fdfile;

	/* COMEÇO DA INTERFACE DE UM SOCKET */
					//	socket(IPV4, TCP, IP)
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_server < 0){
		perror("socket");
		exit(1);
	}

	/* CONFIGURANDO AS OPÇÕES */
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8080);

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
			printf("----------------------------------+\nObtendo Conexão para o Cliente.   |\n----------------------------------+\n");
			
			
			if(!fork()){
				/* PROCESSO FILHO */
				close(fd_server);		
				/* LIMPAR BUFFER */
				memset(buf, 0, 2048);
				read(fd_client, buf, 2047);

				/* GET VERBOSE */
				printf("%s\n", buf);
				
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
					fdimg = open(bufConcat, O_RDONLY);
					sendfile(fd_client, fdimg, NULL, fsize(bufConcat));
					close(fdimg);
				}
				else if(filename == NULL){

					/* ENVIA PARA O CLIENTE VISUALIZAR a PAGINA */
					send(fd_client,webpage,sizeof(webpage),on);
					snprintf(bufConcat, sizeof bufConcat, "%s%s", rootDir, rootFile);
					fdfile=open(bufConcat,O_RDONLY);
					sendfile(fd_client,fdfile,NULL,300);
					close(fdfile);			
				}else{
					printf("	Acess: [%s]\n", filename);
					send(fd_client,webpage,sizeof(webpage),on);
					fdfile=open(bufConcat,O_RDONLY);
					sendfile(fd_client,fdfile,NULL,300);
					close(fdfile);	
				}
				close(fd_client);
				printf("\nConexão Encerrada!\n---------------------------------->\n\n");
				exit(0);
			}
			
			/* PROCESSO PAI */
			close(fd_client);
	}

	return 0;
}
