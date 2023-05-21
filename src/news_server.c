#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 1024 // Tamanho do buffer
#define TOPICSLEN 10

typedef struct topic_node
{
    int id;
    char Topic[21];
    char ip[40];
    struct topic_node *next;
} TopicNode;

typedef struct subscription
{
    TopicNode *topic_node;
    struct subscription *next;
} Subscription;

typedef struct user_node
{
    char username[21];
    char password[21];
    char type[15];
    Subscription *subscriptions;
    struct user_node *next;
} UserNode;

UserNode *root, **head = &root;
TopicNode *topics;

int topiccount = 0;

int serverstate = 1;

char *porto_config;
char *porto_noticias;

pthread_t thread_id;
pthread_mutex_t mutex_shm = PTHREAD_MUTEX_INITIALIZER;

void *handle_udp(void *arg);
void *handle_tcp(void *p_client_socket);

TopicNode *get_topic(int id);

void tcp_server();

void read_config_file(const char *config_file);
int add_user(const char *username, const char *password, const char *type);
int del(const char *username);

void erro(char *s);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n");
        exit(1);
    }
    porto_noticias = argv[1];
    porto_config = argv[2];
    read_config_file(argv[3]);

    if (pthread_create(&thread_id, NULL, handle_udp, NULL))
        erro("Error: Creating udp thread");

    /* if (signal(SIGINT, cleanup) == SIG_ERR)
     erro("Error: signal");*/

    tcp_server();

    if (pthread_join(thread_id, NULL))
        erro("Error: waiting for a thread to finish");

    return 0;
}

void tcp_server()
{
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(porto_noticias));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");

    if (listen(fd, 5) < 0)
        erro("na funcao listen");

    client_addr_size = sizeof(client_addr);

    while (serverstate)
    {
        client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);

        if (client > 0)
        {
            pthread_t t;

            int *p_client_socket = malloc(sizeof(int));

            *p_client_socket = client;

            if (pthread_create(&t, NULL, handle_tcp, p_client_socket))
                erro("Error: Creating tcp thread");
        }
    }
    close(fd);
}

void *handle_tcp(void *p_client_socket)
{
    int client_socket = *((int *)(p_client_socket));
    free(p_client_socket);

    char buffer[BUFLEN];
    char resposta[BUFLEN]; // variavel na qual é inserida a mensagem a enviar ao cliente
    char ip[50];           // ip conrrespondente ao dominio

    char *cmd_args[5]; // pointer that stores command arguments
    char *token;

    char username[50], password[50];

    UserNode *user = NULL;

    write(client_socket, "Bem-vindo ao servidor. Insere o username e password\n", sizeof("Bem-vindo ao servidor. Insere o username e password\n"));

    int num_args;
    while (serverstate)
    {
        num_args = 0;

        bzero(buffer, BUFLEN); // limpar buffer

        int recv_len = read(client_socket, buffer, BUFLEN - 1);

        buffer[recv_len - 1] = '\0';

        printf("Mensagem recebida:%s\n", buffer);

        // get each command parameter piece
        for (token = strtok(buffer, " "); token != NULL && num_args < 2; token = strtok(NULL, " "))
        {
            cmd_args[num_args++] = token;
        }

        if (num_args != 2)
        {
            write(client_socket, "Deve inserir:\nUsername Password\n", sizeof("Deve inserir:\nUsername Password\n"));
        }

        else
        {
            for (UserNode *atual = root; atual != NULL; atual = atual->next)
            {
                if (strcmp(atual->type, "administrator") != 0 && strcmp(atual->username, cmd_args[0]) == 0 && strcmp(atual->password, cmd_args[1]) == 0)
                {
                    user = atual;
                    break;
                }
            }

            if (user == NULL)
            {
                write(client_socket, "credenciais erradas\n", strlen("credenciais erradas\n"));
            }
            else
            {
                write(client_socket, user->type, sizeof(user->type));
                break;
            }
        }
    }

    // enviar topicos no quais o cliente esta inscrito
    // id;ip;topic

    for (Subscription *atual = user->subscriptions; atual != NULL; atual = atual->next)
    {
        sprintf(resposta, "%d;%s;%s", atual->topic_node->id, atual->topic_node->ip, atual->topic_node->Topic);

        write(client_socket, resposta, strlen(resposta)); // enviar a resposta ao cliente
    }

    write(client_socket, "FIM", sizeof("FIM"));

    while (serverstate)
    {
        int nread;

        // limpar variaveis buffer,resposta e ip
        bzero(buffer, BUFLEN);
        bzero(resposta, BUFLEN);

        // obter a mensagem enviada do cliente
        do
        {
            nread = read(client_socket, buffer, BUFLEN - 1);
        } while (nread < 2);

        buffer[nread - 1] = '\0';

        printf("Um cliente enviou: %s\n", buffer);

        char *token = strtok(buffer, " ");

        // verificar se o cliente deseja terminar a sessão
        if (strcmp(token, "EXIT") == 0)
        {
            break;
        }

        if (strcmp(token, "LIST_TOPICS") == 0)
        {
            printf("LIST_TOPICS\n");

            sprintf(resposta, "---Lista de topicos:---\n");
            for (TopicNode *atual = topics; atual != NULL; atual = atual->next)
            {
                printf("%d - %s\n", atual->id, atual->Topic);
            }
            strcat(resposta, "-----------------------\n");
            write(client_socket, resposta, strlen(resposta));
        }
        else if (strcmp(token, "SUBSCRIBE_TOPIC") == 0)
        {
            printf("SUBSCRIBE_TOPIC\n");

            token = strtok(NULL, " ");

            int id = atoi(token);

            TopicNode *p_topic = get_topic(id);

            if (p_topic == NULL)
            {
                write(client_socket, "Topico com esse id não existe\n", strlen("Topico com esse id não existe\n")); // enviar a resposta ao cliente
            }
            else
            {
                Subscription *atual;

                // verificar se já esta inscrito no topico
                for (atual = user->subscriptions; atual != NULL; atual = atual->next)
                {
                    if (atual->topic_node->id == id)
                    {
                        break;
                    }
                }
                if (atual != NULL)
                {
                    printf("ja estava inscrito\n");
                }
                else
                {
                    if (user->subscriptions == NULL)
                    {
                        Subscription *new_node = (Subscription *)malloc(sizeof(Subscription));

                        new_node->topic_node = p_topic;

                        sprintf(resposta, "%d;%s;%s", p_topic->id, p_topic->ip, p_topic->Topic);

                        write(client_socket, resposta, strlen(resposta)); // enviar a resposta ao cliente

                        user->subscriptions = new_node;

                        printf("subscriçao criada\n");
                    }
                    else
                    {
                        Subscription *atual;
                        for (atual = user->subscriptions; atual != NULL; atual = atual->next)
                        {
                            if (atual->next == NULL)
                            {
                                Subscription *new_node = (Subscription *)malloc(sizeof(Subscription));

                                new_node->topic_node = p_topic;

                                sprintf(resposta, "%d;%s;%s", p_topic->id, p_topic->ip, p_topic->Topic);

                                write(client_socket, resposta, strlen(resposta)); // enviar a resposta ao cliente

                                printf("subscriçao criada\n");

                                atual->next = new_node;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else if (strcmp(token, "CREATE_TOPIC") == 0)
        {
            printf("CREATE_TOPIC\n");

            // verificar se existe um topico com o mesmo id

            token = strtok(NULL, " ");

            int id = atoi(token);

            if (topics == NULL)
            {
                TopicNode *new_node = (TopicNode *)malloc(sizeof(TopicNode));

                new_node->id = id;

                token = strtok(NULL, " ");
                strcpy(new_node->Topic, token);

                sprintf(new_node->ip, "224.0.0.%d", ++topiccount);

                topics = new_node;

                printf("topico criado\n");
            }
            else
            {
                TopicNode *atual;

                for (atual = topics; atual != NULL; atual = atual->next)
                {
                    if (atual->id == id)
                    {
                        write(client_socket, "Topico com esse id já existe\n", strlen("Topico com esse id já existe\n")); // enviar a resposta ao cliente
                    }
                    else if (atual->next == NULL)
                    {
                        TopicNode *new_node = (TopicNode *)malloc(sizeof(TopicNode));

                        new_node->id = id;

                        token = strtok(NULL, " ");

                        strcpy(new_node->Topic, token);

                        sprintf(new_node->ip, "239.0.0.%d", ++topiccount);

                        atual->next = new_node;

                        printf("topico criado\n");
                        break;
                    }
                }
            }
        }
        else
        {
            sprintf(resposta, "Comando errado\n");
            write(client_socket, resposta, strlen(resposta)); // enviar a resposta ao cliente
        }
    }

    close(client_socket);

    pthread_exit(NULL);
}

TopicNode *get_topic(int id)
{
    for (TopicNode *atual = topics; atual != NULL; atual = atual->next)
    {
        if (atual->id == id)
        {
            return atual;
        }
    }

    return NULL;
}

void *handle_udp(void *arg)
{
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);

    char *cmd_args[5]; // pointer that stores command arguments
    char *token;
    char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(atoi(porto_config));
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind");
    }

    int num_args; // number of arguments that the user buf has
    UserNode *adminlogged = NULL;

    while (serverstate && adminlogged == NULL)
    {
        num_args = 0;
        while (num_args < 2)
        {
            num_args = 0;

            bzero(buf, BUFLEN); // limpar buffer

            if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
            {
                erro("Erro no recvfrom");
            }

            buf[recv_len - 1] = '\0';

            printf("mensagem recebida:%s\n", buf);

            // get each command parameter piece
            for (token = strtok(buf, " "); token != NULL && num_args < 5; token = strtok(NULL, " "))
            {
                cmd_args[num_args++] = token;
            }
            if (num_args == 1)
            {
                if (sendto(s, "Deve inserir:\nUsername Password\n", strlen("Deve inserir:\nUsername Password\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                {
                    erro("Erro no envio da mensagem");
                }
            }
        }

        for (UserNode *atual = root; atual != NULL; atual = atual->next)
        {
            if (strcmp(atual->type, "administrator") == 0 && strcmp(atual->username, cmd_args[0]) == 0 && strcmp(atual->password, cmd_args[1]) == 0)
            {
                adminlogged = atual;

                if (sendto(s, "Sessão iniciada com sucesso\n", strlen("Sessão iniciada com sucesso\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                {
                    erro("Erro no envio da mensagem");
                }
                if (sendto(s, "\n---Menu---\nADD_USER {username} {password} {administrador/cliente/jornalista}\nDEL {username}\nLIST\nQUIT\nQUIT_SERVER\n\n", strlen("\n---Menu---\nADD_USER {username} {password} {administrador/cliente/jornalista}\nDEL {username}\nLIST\nQUIT\nQUIT_SERVER\n\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                {
                    erro("Erro no envio da mensagem");
                }

                break;
            }
        }

        if (adminlogged == NULL && sendto(s, "credenciais erradas\n", strlen("credenciais erradas\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
        {
            erro("Erro no envio da mensagem");
        }

        while (adminlogged != NULL)
        {
            num_args = 0;
            bzero(buf, BUFLEN); // limpar buffer

            // Espera recepção de mensagem (a chamada é bloqueante)
            if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
            {
                erro("Erro no recvfrom");
            }
            // Para ignorar o restante conteúdo (anterior do buffer)
            buf[recv_len - 1] = '\0';

            printf("\nComando recebido: %s\n", buf);

            // get each command parameter piece
            for (token = strtok(buf, " "); token != NULL && num_args < 5; token = strtok(NULL, " "))
            {
                cmd_args[num_args++] = token;
            }

            if (num_args == 0)
                continue;

            if (strcmp(cmd_args[0], "QUIT") == 0)
            {
                adminlogged = NULL;

                if (sendto(s, "Logout feito com sucesso\n", strlen("Logout feito com sucesso\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                {
                    erro("Erro no envio da mensagem");
                }
            }

            else if (strcmp(cmd_args[0], "ADD_USER") == 0 && num_args == 4)
            {
                if (add_user(cmd_args[1], cmd_args[2], cmd_args[3]))
                {
                    if (sendto(s, "Usuario inserido com sucesso\n", strlen("Usuario inserido com sucesso\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }
                else
                {
                    if (sendto(s, "Erro: Usuario com esse username já existe\n", strlen("Erro: Usuario com esse username já existe\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }
            }
            else if (strcmp(cmd_args[0], "DEL") == 0 && num_args == 2)
            {
                if (strcmp(cmd_args[1], adminlogged->username) == 0)
                {
                    if (sendto(s, "Não é possivel eliminar um usuario com uma conta logada\n", strlen("Não é possivel eliminar um usuario com uma conta logada\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }

                else if (del(cmd_args[1]))
                {
                    if (sendto(s, "Usuario eliminado com sucesso\n", strlen("Usuario eliminado com sucesso\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }
                else
                {
                    if (sendto(s, "Erro: Usuario nao existe\n", strlen("Erro: Usuario nao existe\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }
            }

            else if (strcmp(cmd_args[0], "LIST") == 0)
            {
                for (UserNode *atual = root; atual != NULL; atual = atual->next)
                {
                    sprintf(buf, "%s;%s;%s\n", atual->username, atual->password, atual->type);
                    printf("%s;%s;%s\n", atual->username, atual->password, atual->type);

                    if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&si_outra, slen) == -1)
                    {
                        erro("Erro no envio da mensagem");
                    }
                }
            }
            else if (strcmp(cmd_args[0], "QUIT_SERVER") == 0)
            {
                serverstate = 0;
                adminlogged = NULL;
            }
            else
            {
                // wrong command
                printf("Comando errado\n");
                if (sendto(s, "Comando errado\n", strlen("Comando errado\n"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                {
                    erro("Erro no envio da mensagem");
                }
            }
        }
    }

    close(s);
    pthread_exit(NULL);
}

int del(const char *username)
{
    for (UserNode *last = NULL, *atual = root; atual != NULL; last = atual, atual = atual->next)
    {
        if (strcmp(atual->username, username) == 0)
        {
            if (last != NULL)
            {
                last->next = atual->next;
            }
            else
            {
                root = atual->next;
            }
            free(atual);
            return 1;
        }
    }
    return 0;
}

int add_user(const char *username, const char *password, const char *type)
{
    for (UserNode *last = NULL, *atual = root; atual != NULL; last = atual, atual = atual->next)
    {
        if (strcmp(atual->username, username) == 0)
        { // ja existe um user com esse username
            return 0;
        }
    }

    if (strcmp(type, "administrator") != 0 && strcmp(type, "jornalista") != 0 && strcmp(type, "leitor") != 0)
    {
        printf("tipo errado\n");
        exit(1);
    }

    UserNode *new_node = (UserNode *)malloc(sizeof(UserNode));

    if (new_node == NULL)
        erro("erro a alocar memória para um novo utilizador");

    strcpy(new_node->username, username);
    strcpy(new_node->password, password);
    strcpy(new_node->type, type);
    new_node->next = NULL;

    *head = new_node;
    head = &new_node->next;

    return 1;
}

void read_config_file(const char *config_file)
{
    FILE *fp;

    char line[100], *username, *password, *type;

    fp = fopen(config_file, "r");

    if (fp == NULL)
        erro("Erro ao abrir ficheiro de configurações.\n");

    while (fgets(line, sizeof(line), fp))
    {
        username = strtok(line, ";");
        if (username == NULL)
            erro("Erro ao obter username");

        password = strtok(NULL, ";");
        if (password == NULL)
            erro("Erro ao obter password");

        type = strtok(NULL, "\n");
        if (type == NULL)
            erro("Erro ao obter type");

        add_user(username, password, type);
    }

    fclose(fp);
}

void erro(char *s)
{
    perror(s);
    exit(1);
}
