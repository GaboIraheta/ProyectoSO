# Simulador de reemplazo de páginas: LRU y Clock

Programa de terminal en C que simula los algoritmos de reemplazo de páginas
**LRU** y **Clock** (segunda oportunidad). Muestra paso a paso el estado de los
marcos, los aciertos y fallos, las páginas desalojadas, un resumen con
porcentajes y una comparación entre ambos algoritmos.

## Ejecutar (Windows 11)

El ejecutable ya está compilado en [build/simulador.exe](build/simulador.exe);
no hace falta instalar nada. Abrir **Windows Terminal** o **PowerShell** en la
carpeta del proyecto y ejecutar:

```powershell
# Con un archivo de entrada
.\build\simulador.exe datos_prueba\ejemplo1_clasico.txt

# Interactivo: el programa pide los marcos y las referencias
.\build\simulador.exe
```

(No usar doble clic: la ventana se cierra al terminar.)

Después de cargar los datos aparece un menú:

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
