#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512 // Tamanho do buffer

void print_menu();

void erro(char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUFLEN];

    char input[256];   // input of user
    char *cmd_args[5]; // pointer that stores command arguments
    char *token;
    int num_args; // number of arguments that the user input has

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

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
    // Menu
    printf(" ADD_USER - {username} {password} - Adicionar utilizador\n");
    printf(" DEL - {username} - Eliminar um utilizador\n");
    printf(" LIST - Lista utilizadores\n");
    printf(" QUIT - Sair da consola\n");
    printf(" QUIT_SERVER - Desligar servidor\n");
    printf("Nota: O username só pode ser {administrador/cliente/jornalista}!\n");
}
