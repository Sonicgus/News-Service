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

typedef struct subscription
{
    int id;
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

    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    // bind the socket to the port
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    // join the multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(sub->ip);
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    char msg[BUF_SIZE];

    int addrlen = sizeof(addr);

    while (!turnoff)
    {
        // receive the multicast message
        int nbytes;
        if ((nbytes = recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr *)&addr, &addrlen)) < 0)
        {
            perror("recvfrom");
            exit(1);
        }

        printf("Received multicast message: %s\n", msg);
    }

    // leave the multicast group
    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }
    // close the socket
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

        sscanf(buffer, "%d;%s;%s", new_node->id, new_node->ip, new_node->Topic);

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

        if (strcmp(buffer, "EXIT\n"))
        {
            write(fd, "EXIT", strlen("EXIT")); // enviar a mensagem
            turnoff = 1;
            break;
        }

        if (strcmp(buffer, "LIST_TOPICS") == 0)
        {
            write(fd, buffer, strlen(buffer));
        }
        else if (strcmp(buffer, "SUBSCRIBE_TOPIC") == 0)
        {
            write(fd, buffer, strlen(buffer));

            // se nao deu erro, entao adiciona na lista de topicos subscriptos e começa a receber mensagens do grupo multicast
        }
        else if (type == 2 && strcmp(buffer, "SEND_NEWS") == 0)
        {
            // verificar se o jornalista esta subscrito no topico
            Subscription *atual;
            for (atual = subscriptions; atual != NULL; atual = atual->next)
            {
                if (atual->id == atoi("obtido do user"))
                {
                    for (Subscription *atual = subscriptions; atual != NULL; atual = atual->next)
                    {
                        if (atual->id = id)
                        {
                            // create a UDP socket
                            if ((new_node->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                            {
                                perror("socket");
                                exit(1);
                            }

                            struct sockaddr_in addr;

                            // set up the multicast address structure
                            memset(&addr, 0, sizeof(addr));
                            addr.sin_family = AF_INET;
                            addr.sin_addr.s_addr = inet_addr(new_node->ip);
                            addr.sin_port = htons(5000);

                            // enable multicast on the socket
                            int enable = 1;
                            if (setsockopt(new_node->fd, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0)
                            {
                                perror("setsockopt");
                                exit(1);
                            }
                            // send the multicast message
                            if (sendto(new_node->fd, msg, msglen, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
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
        else if (type == 2 && strcmp(buffer, "CREATE_TOPIC") == 0)
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
