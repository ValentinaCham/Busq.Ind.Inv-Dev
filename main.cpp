#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <cctype>

// Función hash djb2
unsigned long hash_djb2(const std::string& word) {
    unsigned long hash_value = 5381;
    for (char c : word) {
        hash_value = ((hash_value << 5) + hash_value) + c;  // Aplicación del algoritmo djb2
    }
    return hash_value;
}

// Función para procesar archivos de texto y devolver el contenido de cada archivo con un ID único
std::vector<std::pair<int, std::string>> readDocuments(const std::vector<std::string>& filepaths) {
    std::vector<std::pair<int, std::string>> documents;
    int doc_id = 1;
    for (const auto& filepath : filepaths) {
        std::ifstream file(filepath);  // Abrir archivo
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();  // Leer contenido del archivo en un stringstream
            documents.emplace_back(doc_id++, buffer.str());  // Agregar contenido con ID único al vector de documentos
        }
    }
    return documents;
}