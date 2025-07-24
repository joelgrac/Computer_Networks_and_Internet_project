#include "ndn.h"

// Variáveis globais definidas em ndn.h
int numero_vizinhos = 0;
Vizinho lista_vizinhos[MAX_CLIENTES];

void *thread_servidor_tcp(void *arg) {
    ServidorTCPArgs *params = (ServidorTCPArgs *)arg;
    const char *meu_ip = params->ip;
    int minha_porta = params->tcp_port;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[TCP] Erro ao criar socket (servidor)");
        pthread_exit(NULL);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(minha_porta);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("[TCP] Erro no bind");
        close(sockfd);
        pthread_exit(NULL);
    }
    if (listen(sockfd, 5) < 0) {
        perror("[TCP] Erro no listen");
        close(sockfd);
        pthread_exit(NULL);
    }
    printf("[TCP] Servidor do nó em %s:%d aguardando conexões...\n", meu_ip, minha_porta);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            perror("[TCP] Erro no accept");
            continue;
        }
        char ip_cliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, ip_cliente, sizeof(ip_cliente));
        int porta_cliente = ntohs(cliaddr.sin_port);
        printf("[TCP] Conexão recebida de %s:%d\n", ip_cliente, porta_cliente);

        while (1) {
            char buffer[MAX_BUFFER];
            ssize_t n = read(connfd, buffer, MAX_BUFFER - 1);
            if (n <= 0)
                break;
            buffer[n] = '\0';
            char *linha = strtok(buffer, "\n");
            while (linha) {
                printf("[TCP] Recebido: %s\n", linha);
                char cmd[16], param1[128];
                int param2;
                if (sscanf(linha, "%15s", cmd) == 1) {
                    if (strcmp(cmd, "ENTRY") == 0) {
                        if (sscanf(linha, "ENTRY %127s %d", param1, &param2) == 2) {
                            printf("[PROTO] Recebido ENTRY do nó %s:%d\n", param1, param2);
                            if (numero_vizinhos == 0)
                                adicionar_vizinho(param1, param2, 1);
                            else
                                adicionar_vizinho(param1, param2, 0);
                            enviar_mensagem_safe(connfd, meu_ip, minha_porta);
                        }
                    }
                    else if (strcmp(cmd, "SAFE") == 0) {
                        if (sscanf(linha, "SAFE %127s %d", param1, &param2) == 2) {
                            printf("[PROTO] Recebido SAFE do nó %s:%d\n", param1, param2);
                        }
                    }
                    else if (strcmp(cmd, "INTEREST") == 0) {
                        if (sscanf(linha, "INTEREST %127s", param1) == 1) {
                            printf("[PROTO] Recebido INTEREST: %s\n", param1);
                            if (possui_objeto_local(param1))
                                enviar_mensagem_object(connfd, param1);
                            else
                                enviar_mensagem_noobject(connfd, param1);
                        }
                    }
                    else if (strcmp(cmd, "OBJECT") == 0) {
                        if (sscanf(linha, "OBJECT %127s", param1) == 1) {
                            printf("[PROTO] Recebido OBJECT: %s\n", param1);
                            armazenar_objeto_local(param1);
                        }
                    }
                    else if (strcmp(cmd, "NOOBJECT") == 0) {
                        if (sscanf(linha, "NOOBJECT %127s", param1) == 1) {
                            printf("[PROTO] Recebido NOOBJECT: %s\n", param1);
                        }
                    }
                    else if (strcmp(cmd, "LEAVE") == 0) {
                        if (sscanf(linha, "LEAVE %127s %d", param1, &param2) == 2) {
                            printf("[PROTO] Recebido LEAVE do nó %s:%d\n", param1, param2);
                            remover_vizinho(param1, param2);
                            enviar_mensagem_safe(connfd, meu_ip, minha_porta);
                        }
                    }
                    
                    else {
                        printf("[PROTO] Mensagem desconhecida: %s\n", linha);
                    }
                }
                linha = strtok(NULL, "\n");
            }
        }
        close(connfd);
        printf("[TCP] Conexão encerrada com %s:%d\n", ip_cliente, porta_cliente);
    }
    close(sockfd);
    pthread_exit(NULL);
}

int conectar_com_no(const char *ip, int porta) {
    if (strcmp(ip, "0.0.0.0") == 0 || porta == 0) {
        printf("Endereço inválido para conexão.\n");
        return -1;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("[TCP] socket");
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(porta);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("[TCP] Erro no inet_pton");
        close(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[TCP] Erro ao conectar");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void iniciar_cliente_tcp(const char *ip, int port) {
    int sockfd = conectar_com_no(ip, port);
    if (sockfd < 0)
        return;
    const char *msg = "Olá do meu nó via TCP!\n";
    ssize_t bytes_enviados = write(sockfd, msg, strlen(msg));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
    char buffer[MAX_BUFFER];
    ssize_t n = read(sockfd, buffer, MAX_BUFFER - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[TCP] Resposta recebida: %s\n", buffer);
    }
    close(sockfd);
}

void adicionar_vizinho(const char *ip, int tcp, int externo) {
    if (numero_vizinhos >= MAX_CLIENTES) {
        printf("Erro: máximo de vizinhos atingido.\n");
        return;
    }
    strncpy(lista_vizinhos[numero_vizinhos].ip, ip, INET_ADDRSTRLEN - 1);
    lista_vizinhos[numero_vizinhos].ip[INET_ADDRSTRLEN - 1] = '\0';
    lista_vizinhos[numero_vizinhos].tcp = tcp;
    lista_vizinhos[numero_vizinhos].externo = externo;
    numero_vizinhos++;
}

void enviar_mensagem_entry(int sockfd, const char *ip, int tcp) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "ENTRY %s %d\n", ip, tcp);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}

void enviar_mensagem_safe(int sockfd, const char *ip, int tcp) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "SAFE %s %d\n", ip, tcp);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}

void enviar_mensagem_interest(int sockfd, const char *name) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "INTEREST %s\n", name);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}
void enviar_mensagem_object(int sockfd, const char *name) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "OBJECT %s\n", name);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}
void enviar_mensagem_noobject(int sockfd, const char *name) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "NOOBJECT %s\n", name);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}
void enviar_mensagem_leave(int sockfd, const char *ip, int tcp) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "LEAVE %s %d\n", ip, tcp);
    ssize_t bytes_enviados = write(sockfd, buffer, strlen(buffer));
    if (bytes_enviados < 0) {
        perror("[PROTO] Erro ao enviar mensagem ENTRY");
    }
}

