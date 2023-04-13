#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_menu();

int main(int argc, char *argv[])
{
    char input[256];   // input of user
    char *cmd_args[5]; // pointer that stores command arguments
    char *token;
    int num_args; // number of arguments that the user input has

    if (argc != 3)
    {
        printf("news_client {endereço do servidor} {PORTO_NOTICIAS}\n");
        exit(1);
    }

    print_menu();

    while (1)
    {
        num_args = 0;

        printf("> ");

        // get the input from the user
        fgets(input, sizeof(input), stdin);

        // remove \n character from the user input
        input[strcspn(input, "\n")] = '\0';

        // get each command parameter piece
        for (token = strtok(input, " "); token != NULL && num_args < 5; token = strtok(NULL, " "))
        {
            cmd_args[num_args++] = token;
        }

        if (strcmp(cmd_args[0], "QUIT") == 0)
            break;
        else if (strcmp(cmd_args[0], "ADD_USER") == 0 && num_args == 4)
        {
        }
        else if (strcmp(cmd_args[0], "DEL") == 0 && num_args == 2)
        {
        }

        else if (strcmp(cmd_args[0], "LIST") == 0)
        {
        }
        else if (strcmp(cmd_args[0], "QUIT_SERVER") == 0)
        {
        }
        else
        {
            // wrong command
            printf("ERROR\n");
        }
    }

    return 0;
}

void print_menu()
{
    // print the menu
    printf("\nMenu (comando - descricao)\n");
    printf(" exit - Sair\n");
    printf(" stats\n");
    printf(" reset\n");
    printf(" sensors\n");
    printf(" add_alert [id] [chave] [min] [max]\n");
    printf(" remove_alert [id]\n");
    printf(" list_alerts\n");
    printf("escolha o que deseja fazer:\n");
}