#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("news_client {endereço do servidor} {PORTO_NOTICIAS}\n");
        exit(1);
    }

    return 0;
}
