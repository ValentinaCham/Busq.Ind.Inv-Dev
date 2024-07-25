#include <emscripten.h>
#include <emscripten/bind.h>
#include <iostream>
#include <fstream>
#include <string>

// Utilizar MEMFS para el almacenamiento en el navegador

void upload_file(uint8_t* data, int size, std::string filename) {
    // Crear la ruta del archivo dentro del sistema de archivos de Emscripten
    std::string folder = "/archivos"; // Directorio virtual en MEMFS
    std::string file_path = folder + "/" + filename;

    // Crear el directorio si no existe (en MEMFS)
    if (EM_ASM_INT({ return FS.analyzePath(UTF8ToString($0)).exists ? 1 : 0; }, folder.c_str()) == 0) {
        EM_ASM({ FS.mkdir(UTF8ToString($0)); }, folder.c_str());
    }

    // Abrir el archivo para escribir en binario
    std::ofstream file(file_path, std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<char*>(data), size);
        file.close();
        std::cout << "Archivo guardado exitosamente: " << file_path << std::endl;
    } else {
        std::cerr << "Error al guardar el archivo." << std::endl;
    }
}

// Vincular la función de C++ para ser usada en JavaScript
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("upload_file", &upload_file, emscripten::allow_raw_pointer());
}
