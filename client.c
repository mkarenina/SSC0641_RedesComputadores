/*
        SSC0641 - Redes de Computadores
              Trabalho 1
        Arquivo Cliente

        Leonardo Prado Dias
        arianna Karenina de A. Flores - 10821144
        Nicolas Ribeiro Batistuti - 10408351
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define MESSAGE_BUFFER 500
#define USERNAME_BUFFER 10
//#define PORT 3024

char sendBuffer[MESSAGE_BUFFER]; //buffer de envio de mensagens
char receiveBuffer[MESSAGE_BUFFER]; //buffer de recebimento de mensagens
char username[USERNAME_BUFFER]; //nome de usuario


/*
struct responsavel por guardar informacoes da troca de mensagens
*/
struct messageInfo{
    int fd;                     //descritor de socket
    char user[USERNAME_BUFFER]; //nome de usuario
    int opt;                    // 0 = entrou no chat, 1 = saiu do chat, 2 = enviar mensagem, 3 = comecou o jogo e 4 = terminou o jogo
    int onLineNum;              //qtd de usuarios online
};


struct messageInfo *sentMessage, *receivedMessage;

int socket_fd;     //descritor do socket
int gameOwner = 0; //0 jogo nao iniciado, 1 jogo iniciado
int isPlaying = 0; //verifica se o cliente esta no jogo

/** funcao responsavel por obter o conteudo que esta depois da string "/play"
 */
void getWord(char str[], char word[], int start){
    int i = 0;
    for(int j = start; j < strlen(str); j++){
        word[i] = str[j];
        i++;
    }
    word[strlen(word)-1] = '\0';
}

/** Funcao que apenas fecha o socket */
void checkEnd(){
    close(socket_fd);
}

/**Funcao que faz a conexão com servidor e reporta se houve sucesso ou nao */
void *connectServer(int fd, struct sockaddr_in *address){
    int resp = connect(fd, (struct sockaddr *)address, sizeof(*address));
    if(resp < 0){    //Erro ao realizar a conexão
        
        printf("Erro: %s\n", strerror(errno));
        atexit(checkEnd);
        exit(1);
    }
    else{           //Conexao bem sucedida 
        
        printf("Conectado em %s:%d", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
    }
}

/** Função que processa e envia todo o tipo de mensagem enviada seja ela texto ou comando de jogo */
void *receive(void *msg){
    while(1){       //permanece em loop
        sentMessage = (struct messageInfo *)sendBuffer;
        sentMessage->fd = socket_fd;         //associa o descritor do socket do cliente com a mensagem enviada
        strcpy(sentMessage->user, username); //copia o nome de usuario do cliente que enviou a mensagem
        fgets(sendBuffer + sizeof(struct messageInfo), MESSAGE_BUFFER - sizeof(struct messageInfo), stdin);   //obtem texto digitado pelo cliente

        
        if(strcmp(sendBuffer + sizeof(struct messageInfo), "/q\n") == 0){  //caso onde o cliente encerra a sessao com o comando /q
            sentMessage->opt = 1;                                          //usuario desconectou do servidor
            if(send(socket_fd, sendBuffer, MESSAGE_BUFFER, 0) == -1)       //avisa os demais clientes que um cliente desconectou
                printf("Erro - Send: %s\n", strerror(errno));
            atexit(checkEnd);
            exit(0);
        } else{  //caso onde cliente envia mensagem de texto a todos no chat
                    sentMessage->opt = 2;
                    printf("%s> ", username);
                    }

        
        if(send(socket_fd, sendBuffer, MESSAGE_BUFFER, 0) == -1){ //Verifica se houve erros na troca de mensagens
            printf("Erro - Send: %s\n", strerror(errno));
            }

        bzero(sendBuffer, MESSAGE_BUFFER); //seta os bytes para 0
        
    }
    return NULL;
}

/*
Funcao princial responsavel por iniciar o socket, a conexao com o servidor e as devidas threads
bem como ficar verificando as novas mensagens recebidas
*/
int main(int argc, char *argv[]){
    int port;
    struct sockaddr_in address, cl_addr;
    char *server_address;
    int response;
    pthread_t thread;

    port = atoi(argv[1]);          //numero da porta
    address.sin_family = AF_INET; //familia de endereços
    address.sin_port = htons(port); //define porta em que o cliente se conectara
    address.sin_addr.s_addr = INADDR_ANY; //indica que vai atender todas as requisicoes para a porta especificada
    socket_fd = socket(PF_INET, SOCK_STREAM, 0); //cria o socket

    
    if(socket_fd == -1){ //verifica se o socket foi criado com sucesso ou nao
        printf("Erro: %s.\n", strerror(errno));
        atexit(checkEnd);
        exit(1);
    }

    
    do{//obtem o nome de usuario
        fflush(stdin);
        printf("Nome de usuario: ");
        fgets(username, USERNAME_BUFFER+2, stdin);
        if(username[strlen(username)-1] == '\n')
            username[strlen(username) - 1] = 0; //remove  o "\n"
        if(strlen(username) > USERNAME_BUFFER){ //limita o numero de caracteres do nome de usuario
            printf("Digite um username com %d ou menos caracteres.\n", USERNAME_BUFFER);
            getchar(); //remove o "\n" para o fgets funcionar
        }
    }while(strlen(username) > USERNAME_BUFFER);

    
    connectServer(socket_fd, &address);  //chama a funcao que conecta ao servidor

    
    pthread_create(&thread, NULL, receive, NULL); //cria thread responsavel por receber as mensagens
    
    
    while(1){ //Loop responsavel por receber mensagens referentes a outros usuarios      
        bzero(receiveBuffer, MESSAGE_BUFFER);//Limpa o receiveBuffer e define os bytes para 0
        if(recv(socket_fd, receiveBuffer, MESSAGE_BUFFER, 0) == -1) //recebe as mensagens do servidor e verifica se ta tudo certo
            fprintf(stderr, "%s\n", strerror(errno));

        receivedMessage = (struct messageInfo *)receiveBuffer;
        if(receivedMessage->opt == 0 && receivedMessage->onLineNum != 0){ //indica que o cliente acabou de entrar no servidor 
            printf("\nUsuario com ID %d entrou no chat\n", receivedMessage->fd);
            printf("%s> ", username);
        }
        else if(receivedMessage->opt == 1){//indica que o cliente saiu do servidor
            printf("\nUsuario com ID %d saiu do chat\n", receivedMessage->fd);
            printf("%s> ", username);
        }
        else if(receivedMessage->opt == 2){//indica que o cliente recebera mensagem e a exibira      
            printf("\n%s> %s", receivedMessage->user, receiveBuffer+sizeof(struct messageInfo));
            printf("%s> ", username);
        }
        fflush(stdout);
    }
    
    atexit(checkEnd);//Fecha o socket
    pthread_exit(&thread); //Finaliza a thread

    return 0;
}