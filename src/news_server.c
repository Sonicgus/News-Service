#include <stdio.h>
#include <string.h>
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

void read_config_file(const char *config_file);
void add_user(const char *username, const char *password, const char *type);
void del(const char *username);

UserNode *root, **head = &root;

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

    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);

    char *cmd_args[5]; // pointer that stores command arguments
    char *token;
    int num_args; // number of arguments that the user buf has
    char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(atoi(argv[2]));
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind");
    }

    ////////////////////
    int notlogged = 1;
    int serverstate = 1;
    while (serverstate)
    {

        while (notlogged)
        {
            num_args = 0;
            bzero(buf, BUFLEN); // limpar buffer

            if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
            {
                erro("Erro no recvfrom");
            }
            // Para ignorar o restante conteúdo (anterior do buffer)
            buf[recv_len - 1] = '\0';

            // get each command parameter piece
            for (token = strtok(buf, " "); token != NULL && num_args < 5; token = strtok(NULL, " "))
            {
                cmd_args[num_args++] = token;
            }

            for (UserNode *atual = root; atual != NULL; atual = atual->next)
            {
                if (strcmp(atual->username, cmd_args[0]) == 0)
                {
                    if (strcmp(atual->password, cmd_args[1]) == 0)
                    {
                        if (sendto(s, "Sessão iniciada com sucesso", strlen("Sessão iniciada com sucesso"), 0, (struct sockaddr *)&si_outra, slen) == -1)
                        {
                            erro("Erro no envio da mensagem");
                        }
                        notlogged = 0;
                        break;
                    }
                }
            }
            if (sendto(s, "credenciais erradas", strlen("credenciais erradas"), 0, (struct sockaddr *)&si_outra, slen) == -1)
            {
                erro("Erro no envio da mensagem");
            }
        }

        /////////////////////
        while (!notlogged)
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

            printf("Mensagem recebida: %s", buf);

            // get each command parameter piece
            for (token = strtok(buf, " "); token != NULL && num_args < 5; token = strtok(NULL, " "))
            {
                cmd_args[num_args++] = token;
            }

            if (strcmp(cmd_args[0], "QUIT") == 0)
                notlogged = 1;
            else if (strcmp(cmd_args[0], "ADD_USER") == 0 && num_args == 4)
            {
                add_user(cmd_args[1], cmd_args[2], cmd_args[3]);
            }
            else if (strcmp(cmd_args[0], "DEL") == 0 && num_args == 2)
            {
                del(cmd_args[1]);
            }

            else if (strcmp(cmd_args[0], "LIST") == 0)
            {
                for (UserNode *atual = root; atual != NULL; atual = atual->next)
                {
                    printf("%s;%s;%s\n", atual->username, atual->password, atual->type);
                }
            }
            else if (strcmp(cmd_args[0], "QUIT_SERVER") == 0)
            {
                serverstate = 0;
                break;
            }
            else
            {
                // wrong command
                printf("Comando errado\n");
            }
        }
    }
    close(s);
    return 0;
}

void del(const char *username)
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
            return;
        }
    }
}

void add_user(const char *username, const char *password, const char *type)
{
    UserNode *new_node = (UserNode *)malloc(sizeof(UserNode));

    strcpy(new_node->username, username);
    strcpy(new_node->password, password);
    strcpy(new_node->type, type);
    new_node->next = NULL;

    *head = new_node;
    head = &new_node->next;
}

void read_config_file(const char *config_file)
{
    FILE *fp;

    char line[100], *username, *password, *type;

    fp = fopen(config_file, "r");

    if (fp == NULL)
    {
        printf("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp))
    {
        username = strtok(line, ";");
        if (username == NULL)
        {
            printf("Erro ao obter username");
            exit(1);
        }

        password = strtok(NULL, ";");
        if (password == NULL)
        {
            printf("Erro ao obter username");
            exit(1);
        }

        type = strtok(NULL, "\n");
        if (type == NULL)
        {
            printf("Erro ao obter username");
            exit(1);
        }

        add_user(username, password, type);
    }
    fclose(fp);
}
