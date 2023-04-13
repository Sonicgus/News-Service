#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct user_node
{
    char username[21];
    char password[21];
    char type[15];
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
    UserNode *head = root;
    for (head = root; head != NULL; head = head->next)
    {
        printf("Username: %s\n", head->username);
        printf("Password: %s\n", head->password);
        printf("Type: %s\n\n", head->type);
    }

    return 0;
}

void read_config_file(const char *config_file)
{
    FILE *fp;
    UserNode *new_node, **head = &root;
    char line[100];

    fp = fopen(config_file, "r");

    if (fp == NULL)
    {
        printf("Erro ao abrir ficheiro de configurações.\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp))
    {
        new_node = (UserNode *)malloc(sizeof(UserNode));

        strcpy(new_node->username, strtok(line, ";"));
        strcpy(new_node->password, strtok(NULL, ";"));
        strcpy(new_node->type, strtok(NULL, "\n"));
        new_node->next = NULL;

        *head = new_node;
        head = &new_node->next;
    }

    fclose(fp);
}
