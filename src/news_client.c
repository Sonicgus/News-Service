// news_client {endereço do servidor} {PORTO_NOTICIAS}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// tamanho do buffer
#define BUF_SIZE 1024
#define MCPORT 6000

typedef struct subscription
{
    int id;
    char ip[40];
    char Topic[21];
    struct sockaddr_in addr;
    pthread_t thread_id;
    struct subscription *next;
} Subscription;

Subscription *subscriptions;

int turnoff = 0;

int type = 0;

void erro(char *msg);

void *multicast(void *topic_sub)
{
    Subscription *sub = ((Subscription *)(topic_sub));
    ///////////////////////////////////////////
    struct sockaddr_in addr;
    int addrlen, sock, cnt;

    char message[50];

    /*set up socket*/

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }
    int multicastTTL = 255;

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)))
    {
        perror("socketopt");
        exit(1);
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(MCPORT);

    ////////////////////recive///////////////////////
    struct ip_mreq mreq;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = inet_addr(sub->ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt mreq");
    }

    while (!turnoff)
    {
        // receive the multicast message
        int nbytes;
        if ((nbytes = recvfrom(sock, message, sizeof(message), 0, (struct sockaddr *)&addr, &addrlen)) < 0)
        {
            perror("recvfrom");
            exit(1);
        }

        if (nbytes < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        if (nbytes == 0)
        {
            break;
        }

        printf("Received multicast message: %s\n", message);
    }

    ///////////////////////////////////////////

    close(sock);
}

int main(int argc, char *argv[])
{
    char endServer[100];
    int fd;
    struct sockaddr_in addr;
    struct hostent *hostPtr;
    char buffer[BUF_SIZE]; // adicionado variavel buffer

    if (argc != 3)
    { // numero de argumentos necessário é igual a 3
        printf("news_client <host> <port>\n");
        exit(-1);
    }

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");

    while (!turnoff)
    {
        scanf("%s", buffer);               // pedir ao usuário a mensagem a enviar
        write(fd, buffer, strlen(buffer)); // enviar a mensagem

        if (strcmp(buffer, "EXIT\n"))
        {
            write(fd, "EXIT", strlen("EXIT"));
            turnoff = 1;
            break;
        }

        bzero(buffer, BUF_SIZE);
        read(fd, buffer, BUF_SIZE - 1);
        printf("%s", buffer);

        if (strcmp(buffer, "EXIT"))
        {
            turnoff = 1;
            break;
        }
        if (strcmp(buffer, "leitor\n"))
        {
            type = 1;
            break;
        }
        if (strcmp(buffer, "jornalista\n"))
        {
            type = 2;
            break;
        }
    }

    printf("Sessão iniciada com sucesso!\n");

    // receber topicos nos quais esta inscrito

    Subscription *atual = subscriptions;
    while (1)
    {
        bzero(buffer, BUF_SIZE);
        read(fd, buffer, BUF_SIZE - 1);
        // printf("%s", buffer);

        if (strcmp(buffer, "FIM"))
        {
            break;
        }

        Subscription *new_node = (Subscription *)malloc(sizeof(Subscription));

        sscanf(buffer, "%d;%s;%s", &new_node->id, new_node->ip, new_node->Topic);

        // set up the multicast address structure
        memset(&atual->addr, 0, sizeof(atual->addr));
        atual->addr.sin_family = AF_INET;
        atual->addr.sin_addr.s_addr = INADDR_ANY;
        atual->addr.sin_port = htons(5000);

        if (pthread_create(&atual->thread_id, NULL, multicast, atual))
            erro("Error: Creating udp thread");

        atual = atual->next;
    }

    // menu
    printf("---MENU---\nLIST_TOPICS\nSUBSCRIBE_TOPIC <id do tópico>\n");

    if (type == 2)
    {
        printf("CREATE_TOPIC <id do tópico> <título do tópico>\nSEND_NEWS <id do tópico> <noticia>\n");
    }

    while (!turnoff)
    {
        bzero(buffer, BUF_SIZE);        // limpar o buffer preenchendo com zeros
        read(fd, buffer, BUF_SIZE - 1); // guardar a informação recebida no buffer
        printf("%s", buffer);           // imprimir a menssagem recebida
        char *token = strtok(buffer, " ");

        if (strcmp(token, "EXIT\n"))
        {
            write(fd, "EXIT", strlen("EXIT")); // enviar a mensagem
            turnoff = 1;
            break;
        }

        if (strcmp(token, "LIST_TOPICS") == 0)
        {
            write(fd, buffer, strlen(buffer));
        }
        else if (strcmp(token, "SUBSCRIBE_TOPIC") == 0)
        {
            write(fd, buffer, strlen(buffer));

            // se nao deu erro, entao adiciona na lista de topicos subscriptos e começa a receber mensagens do grupo multicast
        }
        else if (type == 2 && strcmp(token, "SEND_NEWS") == 0)
        {
            token = strtok(NULL, " ");
            int id = atoi(token);
            token = strtok(NULL, " ");
            // verificar se o jornalista esta subscrito no topico
            Subscription *atual;
            for (atual = subscriptions; atual != NULL; atual = atual->next)
            {
                if (atual->id == atoi("obtido do user"))
                {
                    for (Subscription *atual = subscriptions; atual != NULL; atual = atual->next)
                    {
                        if (atual->id == id)
                        { // esta subscrito

                            struct sockaddr_in addr;
                            int addrlen, sock, cnt;

                            char message[50];

                            /*set up socket*/

                            sock = socket(AF_INET, SOCK_DGRAM, 0);

                            if (sock < 0)
                            {
                                perror("socket");
                                exit(1);
                            }
                            int multicastTTL = 255;

                            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)))
                            {
                                perror("socketopt");
                                exit(1);
                            }

                            bzero((char *)&addr, sizeof(addr));
                            addr.sin_family = AF_INET;
                            addr.sin_addr.s_addr = htons(INADDR_ANY);
                            addr.sin_port = htons(MCPORT);
                            /////////send message//////

                            addr.sin_addr.s_addr = inet_addr(atual->ip);

                            sprintf(message, "%s\n", token);

                            cnt = sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&addr, addrlen);

                            if (cnt < 0)
                            {
                                perror("sendto");
                                exit(1);
                            }

                            break;
                        }
                    }
                }
            }
            if (atual == NULL)
            {
                printf("Você não está inscrito em nenhum Topico com esse id\n");
            }
        }
        else if (type == 2 && strcmp(token, "CREATE_TOPIC") == 0)
        {
            write(fd, buffer, strlen(buffer));
        }
        else
        {
            printf("Comando errado\n");
        }
    }

    close(fd);
    exit(0);
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(1);
}
