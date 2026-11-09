# Simulador de reemplazo de páginas: LRU y Clock

Programa de terminal en C que simula los algoritmos de reemplazo de páginas
**LRU** y **Clock** (segunda oportunidad). Muestra paso a paso el estado de los
marcos, los aciertos y fallos, las páginas desalojadas, un resumen con
porcentajes y una comparación entre ambos algoritmos.

## Requisitos

- Linux (o cualquier sistema tipo POSIX)
- `gcc` y `make`

En Fedora: `sudo dnf install gcc make`. En Debian/Ubuntu: `sudo apt install build-essential`.

## Compilar

Desde la raíz del proyecto:

```bash
make
```

Esto genera el ejecutable `build/simulador`. Otros comandos:

| Comando      | Qué hace                                              |
|--------------|-------------------------------------------------------|
| `make`       | Compila el simulador                                  |
| `make test`  | Compila y ejecuta las pruebas (LRU, Clock, simulador) |
| `make windows` | Genera `build/simulador.exe` para Windows (ver abajo) |
| `make clean` | Borra los binarios generados                          |

### Windows 11

El código es portable (no depende de funciones exclusivas de Linux), y hay dos
formas de obtener el `.exe`:

**A) Desde Linux (compilación cruzada).** Instalar MinGW-w64 y compilar:

```bash
sudo dnf install mingw64-gcc          # Fedora
# sudo apt install gcc-mingw-w64-x86-64   # Debian/Ubuntu
make windows
```

Se genera `build/simulador.exe`, enlazado estáticamente, así que se puede copiar
a cualquier Windows 11 sin instalar nada más.

**B) Directamente en Windows.** Instalar [MSYS2](https://www.msys2.org/), abrir
la terminal "MSYS2 UCRT64" y ejecutar:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc make
make
```

Para ejecutarlo en Windows, usar **Windows Terminal** o PowerShell:

```powershell
.\build\simulador.exe datos_prueba\ejemplo1_clasico.txt
```

## Ejecutar

Con un archivo de entrada:

```bash
./build/simulador datos_prueba/ejemplo1_clasico.txt
```

O de forma interactiva (el programa pide los datos):

```bash
./build/simulador
```

En ambos casos, después de cargar los datos aparece un menú:

```
1) LRU
2) Clock
3) Ambos (con comparación)
0) Salir
```

El menú se repite hasta elegir `0`, así se pueden probar los algoritmos sobre
los mismos datos sin volver a cargarlos.

## Formato del archivo de entrada

La primera línea útil es el número de marcos (1 a 64) y las siguientes son la
secuencia de referencias (páginas de 0 a 9999, hasta 10 000 referencias).
Se pueden separar con espacios, comas o punto y coma. Las líneas vacías y las
que empiezan con `#` se ignoran.

```
# Número de marcos
3

# Secuencia de referencias
7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
```

En [datos_prueba/](datos_prueba) hay cinco ejemplos listos para usar, con los
resultados esperados en los comentarios de cada archivo.

## Salida

- `FALLO` se muestra en rojo y `ACIERTO` en verde. El color solo se usa en una
  terminal; se desactiva al redirigir la salida o con `NO_COLOR=1`.
- La columna *Desalojada* indica la página que salió de memoria en cada fallo
  (`-` si no hubo desalojo).

## Estructura

| Ruta           | Contenido                                           |
|----------------|-----------------------------------------------------|
| `main.c`       | Programa principal y menú                           |
| `simulator/`   | Ejecución de ambos algoritmos, tablas y comparación |
| `lru/`         | Algoritmo LRU (lista doble + tabla hash)            |
| `clock/`       | Algoritmo Clock (bit R y manecilla)                 |
| `common/`      | Tipos compartidos y lectura de la entrada           |
| `tests/`       | Pruebas automáticas                                 |
| `datos_prueba/`| Archivos de entrada de ejemplo                      |
