
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512 // Tamanho do buffer

typedef struct user_node
{
    char username[21];
    char password[21];
    char type[15];
    struct user_node *next;
} UserNode;

UserNode *root, **head = &root;
pthread_t thread_id[2];
pthread_mutex_t mutex_shm = PTHREAD_MUTEX_INITIALIZER;
char *porto_config;

void cleanup();
void read_config_file(const char *config_file);
int add_user(const char *username, const char *password, const char *type);
int del(const char *username);
void save_users(const char *config_file);

void *udp(void *arg);
void *tcp(void *arg);

void erro(char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n");
        exit(1);
    }
    read_config_file(argv[3]);
    porto_config = argv[2];

    if (pthread_create(&thread_id[0], NULL, udp, NULL))
    {
        perror("Error: Creating udp thread");
        cleanup();
        exit(1);
    }
    if (pthread_create(&thread_id[1], NULL, tcp, NULL))
    {
        perror("Error: Creating tcp thread");
        cleanup();
        exit(1);
    }

    if (signal(SIGINT, cleanup) == SIG_ERR)
    {
        perror("Error: signal");
        cleanup();
        exit(1);
    }

    for (int i = 0; i < 2; i++)
    {
        if (pthread_join(thread_id[i], NULL))
        {
            perror("Error: waiting for a thread to finish");
            exit(1);
        }
    }

    save_users(argv[3]);

    cleanup();

    return 0;
}

void cleanup()
{
    if (pthread_mutex_destroy(&mutex_shm))
    {
        perror("Error: destroy mutex");
    }
}

void *udp(void *arg)
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
    int serverstate = 1;
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
            if (strcmp(atual->type, "administrator") && strcmp(atual->username, cmd_args[0]) == 0 && strcmp(atual->password, cmd_args[1]) == 0)
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

void *tcp(void *arg)
{
    pthread_exit(NULL);
}

void save_users(const char *config_file)
{
    FILE *fp;

    fp = fopen(config_file, "w");

    if (fp == NULL)
    {
        perror("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    fclose(fp);

    fp = fopen(config_file, "a");

    if (fp == NULL)
    {
        perror("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    for (UserNode *atual = root; atual != NULL; atual = atual->next)
    {
        fprintf(fp, "%s;%s;%s\n", atual->username, atual->password, atual->type);
    }

    fclose(fp);
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
        {
            return 0;
        }
    }

    UserNode *new_node = (UserNode *)malloc(sizeof(UserNode));

    if (new_node == NULL)
    {
        perror("erro a alocar memória para um novo utilizador");
        exit(1);
    }

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
    {
        perror("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp))
    {
        username = strtok(line, ";");
        if (username == NULL)
        {
            perror("Erro ao obter username");
            exit(1);
        }

        password = strtok(NULL, ";");
        if (password == NULL)
        {
            perror("Erro ao obter password");
            exit(1);
        }

        type = strtok(NULL, "\n");
        if (type == NULL)
        {
            perror("Erro ao obter type");
            exit(1);
        }

        add_user(username, password, type);
    }
    fclose(fp);
}
