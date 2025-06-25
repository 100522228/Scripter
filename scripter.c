#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

const int max_line = 1024;
const int max_commands = 10;
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15

char * argvv[max_args];
char * filev[max_redirections];
int background = 0; 

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);
    while (token != NULL && i < max_tokens - 1) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return i;
}
/*Handeler para no dejar procesos zombi*/
void procesos_zombi(int signum) {
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0); 
}

/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i + 1] = "Manchado";
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = "Manchado";
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i++] = NULL;
            args[i + 1] = "Manchado";
        }
    }
    for (int i=0; strcmp(args[i], "\0") == 0; i++){
        if(strcmp(args[i], "Manchado") == 0) {
            args[i] = NULL;
        } 
    }
}

/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea) {
    if (strlen(linea) == 0) {
        return 0;
    }
    background = 0;
    char *comandos[max_commands];
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);
    if (num_comandos == 0) {
        perror("No hay comandos.\n");
        return 0;
    }
    int fd_lectura_prev = -1;

    //Check if background is indicated
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        //remove character 
        *pos = '\0';
    }

    for (int i = 0; i < num_comandos; i++) {
        tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
        procesar_redirecciones(argvv);
        for(int arg = 1; arg < max_args; arg++)
            if(argvv[arg] != NULL)
                argvv[arg]= strtok(argvv[arg], "\"");

        // Crear pipes si hay más de un comando
        int fd_pipe[2];
        if (i < num_comandos - 1) {
            // Pipe para conectar la salida del comando actual con la entrada del siguiente
            if (pipe(fd_pipe) == -1) {
                perror("Error al crear pipe\n");
                exit(-1);
            }
        }

        pid_t pid_hijo = fork();
        if (pid_hijo < 0) {
            perror("Fallo a la hora de crear el hijo\n");
            exit(-1);
        }
        if (pid_hijo == 0) { // Proceso hijo
            // Redirección de entrada (si no es el primer comando)
            if (i > 0) {
                if (dup2(fd_lectura_prev, STDIN_FILENO) == -1) {
                    close(fd_lectura_prev);
                    perror("Error redirigiendo la entrada\n");
                    exit(-1);
                }
                close(fd_lectura_prev);
            }

            // Redirección de salida (si no es el último comando)
            if (i < num_comandos - 1) {
                if (dup2(fd_pipe[1], STDOUT_FILENO) == -1) {
                    close(fd_pipe[1]);
                    perror("Error redirigiendo la salida\n");
                    exit(-1);
                }
                close(fd_pipe[1]);
            }

            // Redirecciones explícitas
            if (filev[0] != NULL && i == 0) { // Redirigir stdin desde archivo
                int fd_in = open(filev[0], O_RDONLY);
                if (fd_in == -1) {
                    perror("Error abriendo un archivo de entrada.\n");
                    exit(-1);
                }
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    close(fd_in);
                    perror("Error redirigiendo la salida\n");
                    exit(-1);
                }
                close(fd_in);
            }
            if (filev[1] != NULL && i == num_comandos - 1) { // Redirigir stdout a archivo
                int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out == -1) {
                    perror("Error abriendo archivo de salida\n");
                    exit(-1);
                }
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    perror("Error en dup2 para salida\n");
                    close(fd_out);
                    exit(-1);
                }
                close(fd_out);
            }
            if (filev[2] != NULL) { // Redirigir stderr a archivo
                int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_err == -1) {
                    perror("Error abriendo archivo de salida\n");
                    exit(-1);
                }
                if (dup2(fd_err, STDERR_FILENO) == -1) {
                    close(fd_err);
                    perror("Error redirigiendo la salida\n");
                    exit(-1);
                }
                close(fd_err);
            }

            // Ejecutar el comando
            execvp(argvv[0], argvv);
            perror("Fallo a la hora de ejecutar el hijo.\n");
            exit(-1);
        } 

        if (i == num_comandos - 1 && background == 1){
            printf("%d",pid_hijo);
        }

        // Cierra el descriptor de pipe de la iteración anterior que ya ha sido utilizado para leer los datos 
        if (i > 0) {
            close(fd_lectura_prev);
        }
        if (i < num_comandos - 1) {
            close(fd_pipe[1]);
            // Guardar lectura para el siguiente comando
            fd_lectura_prev = fd_pipe[0];
        }

        if (background == 0 && i == num_comandos - 1) {
            wait(NULL);
        }
    }

    return num_comandos;
}

int main(int argc, char *argv[]) {
    struct sigaction act;
    act.sa_handler = procesos_zombi;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        perror("sigaction\n");
        exit(-1);
    }
    if (argc != 2){
        perror("Número parámetros mal.\n");
        exit(-1);
    }
    int final_b1 = 0, final_b2 = 0, fd_lectura, lectura;
    char linea[max_line];
    linea[0] = '\0';
    char primeralinea[] = "## Script de SSOO\n";
    char linea1[19];
    char caracter;
    int pos = 0;

    /* STUDENTS CODE MUST BE HERE */

    fd_lectura = open(argv[1],O_RDONLY);
    if (fd_lectura == -1){
        perror("Error al abrir el fichero de lectura\n");
        exit(-1);
    }

    int leido = read(fd_lectura, linea1, 18);
    if (leido == -1){
        perror("Error al leer el achivo de entrada en el main.");
        exit(-1);
    }

    linea1[leido] = '\0';
    if (strcmp(linea1, primeralinea) != 0){
        perror("El fichero comienza erroneamente\n");
        exit(-1);
    }
    
    while (final_b1 == 0){
        while (final_b2 == 0){
            lectura = read(fd_lectura, &caracter, 1);
            if (lectura == -1){
                perror("Error al leer el achivo de entrada en el main.");
                exit(-1);
            }
            if (caracter == '\n' || lectura == 0){
                linea[pos] = '\0';
                final_b2 = 1;
                if (lectura == 0){
                    final_b1 = 1;
                }else if (caracter == '\n' && strlen(linea) == 0)
                {
                    perror("Línea vacía\n");
                    exit(-1);
                }
                
            }
            else {
                if (pos < max_line - 1) {
                    linea[pos++] = caracter;
                }
            }
        }
        final_b2 = 0;
        if (pos > 0) {
            procesar_linea(linea);
        }
        pos = 0;
        linea[0] = '\0';
    }
    return 0;
}