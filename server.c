/*
        SSC0641 - Redes de Computadores
              Trabalho 1
        Arquivo Servidor

        Leonardo Prado Dias
        Marianna Karenina de A. Flores - 10821144
        Nicolas Ribeiro Batistuti - 10408351
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define MESSAGE_BUFFER 500 //tamanho maximo do texto da mensagem
#define USERNAME_BUFFER 10 //tamanho maximo do nome de usuario

/*
struct responsavel por guardar informacoes da troca de mensagens
*/
struct messageInfo{
    int fd;                     //descritor de socket
    char user[USERNAME_BUFFER]; //nome de usuario
    int opt;                    //0 = entrou no chat, 1 = saiu do chat, 2 = enviar mensagem e 3 = jogar
    int onLineNum;              //quantidade de usuarios online
};

struct messageInfo *sendMsg, *receiveMsg;

//variaveis globais
int qtdOn = 0;       //quantidade de usuarios online no servidor
int socket_fd;      //descritor do socket
int clients[10];   //lista dos clientes conectados
char sendBuffer[MESSAGE_BUFFER]; //buffer de envio de mensagens
char receiveBuffer[MESSAGE_BUFFER]; //buffer de recebimento de mensagens

/** Funcao que apenas fecha o socket */
void checkEnd(){
    close(socket_fd);
}

/** Funcao que envia uma mensagem para todos os demais clientes online */
void messageClient(char *buffer, int fd){
    for(int i = 0; i < qtdOn; i++){
        if(clients[i] == fd)    //verifica se eh o mesmo cliente que mandou a mensagem e ignora
            continue;
        if(send(clients[i], buffer, MESSAGE_BUFFER, 0) == -1) //se houve erro ele reporta
            printf("%s\n", strerror(errno));
    }
    bzero(buffer, MESSAGE_BUFFER); //seta os bytes para 0
}

/**Função que recebe a mensagem do cliente */
void *receive(void *arg){
    int fd = *(int *)arg; //converte para int

    // loop que fica verificando o recebimento de mensagem
    while(1){
        if(recv(fd, receiveBuffer, MESSAGE_BUFFER, 0) == -1){ //recebe mensagem e verifica se deu erro
            printf("Erro: %s\n", strerror(errno));
            close(fd);
            exit(1);
        }

        receiveMsg = (struct messageInfo *)receiveBuffer;
        receiveMsg->fd = fd; //descritor de socket
        
        //Caso onde o cliente desconectou do servidor
        if(receiveMsg->opt == 1){
            
            qtdOn--; //decrementa quantidade de pessoas online
            receiveMsg->onLineNum = qtdOn;
            printf("Usuario com ID %d desconectou.\n", fd);
            messageClient(receiveBuffer, fd); //envia mensagem aos demais clientes
            close(fd);
            return NULL;
        } //Caso 2,3 e 4 onde ocorerra o jogo 
        else if (receiveMsg->opt == 2 || receiveMsg->opt == 3 || receiveMsg->opt == 4){  
            messageClient(receiveBuffer, fd); //manda mensagem aos clientes
        }
    }
    close(fd);
    return NULL;
}

//main
int main(int argc, char *argv[]) {
    int port, clientFD; //numero da porta e descritor do cliente
    struct sockaddr_in address, cl_addr; //endereço associado ao socket e endereço do cliente
    socklen_t length; //tamanho do socket
    pthread_t thread; //thread do servidor

    port = atoi(argv[1]); //numero da porta
    address.sin_family = AF_INET; //familia de endereços
    address.sin_port = htons(port); //define porta em que o servidor "escutará" as conexões
    address.sin_addr.s_addr = INADDR_ANY; //indica que vai atender todas as requisições para a porta especificada
    socket_fd = socket(PF_INET, SOCK_STREAM, 0); //cria o socket


    //Verifica se o socket foi criado com sucesso ou nao 
    if(socket_fd == -1){
        printf("Erro: %s.\n", strerror(errno));
        atexit(checkEnd);
        exit(1);
    }

    //Verifica se houve erro no bind
    if(bind(socket_fd, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) == -1){
        printf("Erro - Bind: %s.\n", strerror(errno));
        atexit(checkEnd);
        exit(1);
    }

    //Verifica se houve erro no listen
    if(listen(socket_fd, 100) == -1){ 
        printf("Erro - Listen: %s\n.", strerror(errno));
        atexit(checkEnd);
        exit(1);
    }
    
    //prints avisando o sucesso na conexão
    printf("Porta conectada: %d\n", port);
    printf("Esperando...\n");

    while(1){
        
        //Verifica se o cliente foi aceito no server
        if((clientFD = accept(socket_fd, (struct sockaddr *)&cl_addr, &length)) == 0){
            printf("Erro - Accept: %s\n", strerror(errno));
            continue;
        } //Verifica se o cliente eh invalido
        else if (clientFD == -1){
            printf("Erro - FD: %s\n", strerror(errno));
            atexit(checkEnd);
            exit(1);
        }

        clients[qtdOn] = clientFD; //armazena o descritor de socket em um vetor
        qtdOn++; //incrementa quantidade de clientes online
        printf("Cliente com socket %d esta conectado!\n", clientFD);

        
        sendMsg = (struct messageInfo *)sendBuffer; //Mensagem a ser enviada 
        sendMsg->fd = clientFD; //envia descritor do cliente
        sendMsg->opt = 0; //envia que opção que indica que o cliente esta conectado
        sendMsg->onLineNum = qtdOn; //envia a quantidade de clientes online

        //Avisa todos os usuarios do servidor que um novo usario está conectado
        for(int i = 0; i < qtdOn; i++){
            if(send(clients[i], sendBuffer, MESSAGE_BUFFER, 0) == -1)
                printf("Erro - Send: %s.\n", strerror(errno));
        }

        bzero(sendBuffer, MESSAGE_BUFFER); //seta bytes para 0

        //Thread que processa as mensagens recebidas
        if(pthread_create(&thread, NULL, receive, &clientFD) != 0){
            printf("Erro ao criar thread.\n");
            atexit(checkEnd);
            exit(1);
        }
    }
    atexit(checkEnd); //Finaliza o socket
    pthread_exit(&thread); //Finaliza a thread

    return 0;
}