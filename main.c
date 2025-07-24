#include "ndn.h"
int sou_salvaguarda = 0;
int g_cacheSize = 0;

int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Uso: %s <cache> <meuIP> <minhaPortaTCP> <serverIP> <serverUDPPort>\n", argv[0]);
        return 1;
    }
    
    g_cacheSize = atoi(argv[1]);
    const char *meu_ip          = argv[2];
    int         minha_porta_tcp = atoi(argv[3]);
    const char *server_ip       = argv[4];
    int         server_port     = atoi(argv[5]);

    // Inicia a thread do servidor TCP
    pthread_t tid;
    ServidorTCPArgs args;
    snprintf(args.ip, sizeof(args.ip), "%s", meu_ip);
    args.tcp_port = minha_porta_tcp;
    if (pthread_create(&tid, NULL, thread_servidor_tcp, &args) != 0) {
        perror("[MAIN] Erro ao criar thread TCP");
        return 1;
    }

    // Loop de comandos
    while (1) {
        printf("\nComandos: join, direct join, create, delete, retrieve, show topology, show names, leave, exit\n> ");
        fflush(stdout);
        char linha[MAX_BUFFER];
        if (!fgets(linha, sizeof(linha), stdin)) {
            break;
        }
        linha[strcspn(linha, "\n")] = '\0';
        if (strncmp(linha, "exit", 4) == 0) {
            interpretar_comando(linha, server_ip, server_port, meu_ip, minha_porta_tcp);
            printf("[MAIN] Encerrando aplicação...\n");
            break;
        }
        interpretar_comando(linha, server_ip, server_port, meu_ip, minha_porta_tcp);
    }
    return 0;
}
