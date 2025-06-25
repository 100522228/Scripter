# Intérprete de comandos y MyGrep

Este proyecto fue desarrollado como parte de una práctica de la asignatura de Sistemas Operativos en la Universidad Carlos III de Madrid.

Contiene dos programas escritos en C diseñados para sistemas **UNIX/Linux**:

- `scripter`: Un intérprete de comandos que ejecuta scripts personalizados.
- `mygrep`: Una versión simplificada del comando `grep` que busca cadenas de texto en archivos.

## ⚙️ Requisitos

- Sistema operativo tipo UNIX (Linux, macOS...).
- Compilador C (por ejemplo, `gcc`).
- Entorno de desarrollo compatible con Makefile.

## 📁 Estructura del proyecto

- `scripter.c`: Implementa un mini intérprete de comandos que ejecuta scripts línea a línea utilizando procesos hijos, tuberías y redirecciones.
- `mygrep.c`: Implementa una versión simple del comando grep que lee un archivo línea por línea y busca una cadena.
- `Makefile`: Automatiza la compilación de ambos programas.

## 🚀 Ejecución

Compilar ambos programas:

```bash
make
```

Ejecutar el intérprete de comandos con un script:

```bash
./scripter < script.txt
```

Ejecutar mygrep:

```bash
./mygrep archivo.txt cadena
```
