#include "ndn.h"
#include <stdio.h>
#include <string.h>

char objetos_locais[MAX_OBJETOS_LOCAIS][MAX_BUFFER];
int  num_objetos_locais = 0;

int possui_objeto_local(const char *nome) {
    for (int i = 0; i < num_objetos_locais; i++) {
        if (strcmp(objetos_locais[i], nome) == 0) {
            return 1;
        }
    }
    return 0;
}

void armazenar_objeto_local(const char *nome) {
    extern int g_cacheSize;
    if (num_objetos_locais >= g_cacheSize) {
        printf("[OBJ] Limite máximo de objetos atingido.\n");
        return;
    }
    if (possui_objeto_local(nome)) {
        printf("[OBJ] Objeto '%s' já existe localmente.\n", nome);
        return;
    }
    strncpy(objetos_locais[num_objetos_locais], nome, MAX_BUFFER - 1);
    objetos_locais[num_objetos_locais][MAX_BUFFER - 1] = '\0';
    num_objetos_locais++;
    printf("[OBJ] Objeto '%s' armazenado.\n", nome);
}

void remover_objeto_local(const char *nome) {
    for (int i = 0; i < num_objetos_locais; i++) {
        if (strcmp(objetos_locais[i], nome) == 0) {
            for (int j = i; j < num_objetos_locais - 1; j++) {
                strcpy(objetos_locais[j], objetos_locais[j + 1]);
            }
            num_objetos_locais--;
            printf("[OBJ] Objeto '%s' removido.\n", nome);
            return;
        }
    }
    printf("[OBJ] Objeto '%s' não encontrado.\n", nome);
}

void listar_objetos_locais(void) {
    printf("[OBJ] Objetos locais (%d):\n", num_objetos_locais);
    for (int i = 0; i < num_objetos_locais; i++) {
        printf("   - %s\n", objetos_locais[i]);
    }
}
