#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct user_node
{
    char username[21];
    char password[21];
    char type[14];
    struct user_node *next;
} UserNode;

void read_config_file(const char *config_file);

UserNode *root;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n");
        exit(1);
    }
    read_config_file(argv[3]);

    return 0;
}

void read_config_file(const char *config_file)
{
    FILE *fp;
    UserNode *new_node, head = root;
    char line[100];

    fp = fopen("usuarios.txt", "r");

    if (fp == NULL)
    {
        printf("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), file))
    {
        new_node = (UserNode *)malloc(sizeof(UserNode));

        strcpy(new_node->username, strtok(line, ";"));
        strcpy(new_node->password, strtok(NULL, ";"));
        strcpy(new_node->type, strtok(NULL, "\n"));
        new_node->next = NULL;

        head = new_node->next;
    }

    fclose(fp);
}
