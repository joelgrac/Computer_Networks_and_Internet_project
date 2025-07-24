#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 59000
#define MAX_BUFFER 1024
#define MAX_NODES 100

typedef struct {
    char net[16];   
    char ip[16];    
    int  tcp;       
} NodeReg;

static NodeReg node_list[MAX_NODES];
static int node_count = 0;

int find_node(const char *net, const char *ip, int tcp) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(node_list[i].net, net) == 0 &&
            strcmp(node_list[i].ip, ip) == 0 &&
            node_list[i].tcp == tcp) {
            return i;
        }
    }
    return -1;
}

int add_node(const char *net, const char *ip, int tcp) {
    if (node_count >= MAX_NODES)
        return -1;
    if (find_node(net, ip, tcp) >= 0)
        return 0; // Já existe
    strncpy(node_list[node_count].net, net, 15);
    node_list[node_count].net[15] = '\0';
    strncpy(node_list[node_count].ip, ip, 15);
    node_list[node_count].ip[15] = '\0';
    node_list[node_count].tcp = tcp;
    node_count++;
    return 1;
}

int remove_node(const char *net, const char *ip, int tcp) {
    int idx = find_node(net, ip, tcp);
    if (idx < 0)
        return 0;
    for (int i = idx; i < node_count - 1; i++) {
        node_list[i] = node_list[i+1];
    }
    node_count--;
    return 1;
}

void build_nodeslist(char *buffer, size_t bufsize, const char *net) {
    snprintf(buffer, bufsize, "NODESLIST %s\n", net);
    for (int i = 0; i < node_count; i++) {
        if (strcmp(node_list[i].net, net) == 0) {
            char linha[64];
            snprintf(linha, sizeof(linha), "%s %d\n", node_list[i].ip, node_list[i].tcp);
            strncat(buffer, linha, bufsize - strlen(buffer) - 1);
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);
    char buffer[MAX_BUFFER];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Erro no bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Servidor de nós rodando na porta %d...\n", SERVER_PORT);

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0,
                             (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (n < 0) {
            perror("Erro ao receber");
            continue;
        }
        buffer[n] = '\0';
        printf("Recebido: %s\n", buffer);

        char cmd[16], net[16], ip[16];
        int tcp;
        char resp[MAX_BUFFER];

        if (sscanf(buffer, "%15s", cmd) == 1) {
            if (strcmp(cmd, "NODES") == 0) {
                if (sscanf(buffer, "NODES %15s", net) == 1) {
                    memset(resp, 0, sizeof(resp));
                    build_nodeslist(resp, sizeof(resp), net);
                    if (strlen(resp) == 0) {
                        snprintf(resp, sizeof(resp), "NODESLIST %s\n", net);
                    }
                    sendto(sockfd, resp, strlen(resp), 0,
                           (struct sockaddr *)&cliaddr, cliaddr_len);
                }
            }
            else if (strcmp(cmd, "REG") == 0) {
                if (sscanf(buffer, "REG %15s %15s %d", net, ip, &tcp) == 3) {
                    int ret = add_node(net, ip, tcp);
                    if (ret >= 0) {
                        const char *ok = "OKREG\n";
                        sendto(sockfd, ok, strlen(ok), 0,
                               (struct sockaddr *)&cliaddr, cliaddr_len);
                    } else {
                        const char *err = "ERROREG\n";
                        sendto(sockfd, err, strlen(err), 0,
                               (struct sockaddr *)&cliaddr, cliaddr_len);
                    }
                }
            }
            else if (strcmp(cmd, "UNREG") == 0) {
                if (sscanf(buffer, "UNREG %15s %15s %d", net, ip, &tcp) == 3) {
                    int ret = remove_node(net, ip, tcp);
                    if (ret > 0) {
                        const char *ok = "OKUNREG\n";
                        sendto(sockfd, ok, strlen(ok), 0,
                               (struct sockaddr *)&cliaddr, cliaddr_len);
                    } else {
                        const char *err = "ERROUNREG\n";
                        sendto(sockfd, err, strlen(err), 0,
                               (struct sockaddr *)&cliaddr, cliaddr_len);
                    }
                }
            }
            else {
                const char *unk = "MSG DESCONHECIDA\n";
                sendto(sockfd, unk, strlen(unk), 0,
                       (struct sockaddr *)&cliaddr, cliaddr_len);
            }
        }
    }
    close(sockfd);
    return 0;
}
