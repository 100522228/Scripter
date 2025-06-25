#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Uso: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el archivo");
        return -1;
    }

    char *cadena_busqueda = argv[2];
    char *buffer = NULL;
    char ch;
    int pos = 0;
    int encontrada = 0;
    ssize_t lectura;

    while ((lectura = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            if (buffer != NULL) {
                buffer[pos] = '\0';
                if (strstr(buffer, cadena_busqueda) != NULL) {
                    printf("%s\n", buffer);
                    encontrada=1;
                }
                free(buffer);
                buffer = NULL;
                pos = 0;
            }
        } else {
            // Reasignar buffer para agregar un nuevo carácter
            char *temp = realloc(buffer, pos + 2); // +1 para nuevo char, +1 para '\0'
            if (temp == NULL) {
                perror("Error de memoria");
                free(buffer);
                close(fd);
                return -1;
            }
            buffer = temp;
            buffer[pos++] = ch;
        }
    }

    // Verificar si quedó algo en buffer sin salto de línea
    if (!encontrada && pos > 0 && buffer != NULL) {
        buffer[pos] = '\0';
        if (strstr(buffer, cadena_busqueda) != NULL) {
            printf("%s", buffer);
            encontrada=1;
        }
    }

    close(fd);
    free(buffer);

    if (encontrada == 0) {
        printf("'%s' not found\n", cadena_busqueda);
    }

    return 0;
}