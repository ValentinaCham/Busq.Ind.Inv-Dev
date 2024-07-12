# Busqueda invertida con Map Reducer

Proyecto que implementa la búsuqeda invertida y optimiza el proceso para  garantizar una mejor búsqueda 

## Explicación

El programa muestra como se leen los archivos txt, para luego mostrar como son asociados los archivos a palabras clave y luego ser reducidos con mapeo y luego ser considerados para la búsqueda.

### Unordered_map

Esta estructura es usada para almacenar una clave string y otra un conjunto **set** de enteros para almacenar ids para los documentos que han sido mapeados,

### Proceso de mapeo

En este proceso se busca cada palabra evaluando cada caracter, si es un espacio, signo de puntuación o palabra a evitar, la cadena se elimina y se continua evaluando los siguientes caracteres para ser guardados en el mapa intermedio

### Mapa intermediate_output

Este mapa almacena la información tal cómo se espera un cadena (palabra clave) que sirve como la key en el mapa, con valores asociados en un conjunto de ids de los documentos leídos.

### Función hashdbj2

Esta función en el código proporciona un codigo hash para asociar a valores numéricos a las cadenas que funcionan cómo índice invertido, 

### Vector lookuptable

Esta estructura almacenará un string (la palabra) y un set de ids de los documentos, la ubicación de las palabras clave en el vector está asociada al valor hashdbj2 para colocarse después de la operación modulo. 

### Función searchWord

Este método hallará el valor hashdbj2 resultante del string ingresado, para acceder al índice del vector lookuptable y así retornar la palabra y su conjunto de documentos id.

## Objetivos

- Uso de conectores como AND, OR y NOT para las búsquedas
- Interfaz para la búsqueda
- Manejo de archivos en cualquier formato en relación a textos (pdf, txt)

## Ejecución

- Compile el archivo main.cpp
- Ejecute el archivo de salida

## Referencias
