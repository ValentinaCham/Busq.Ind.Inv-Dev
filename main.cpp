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

// Función para guardar los resultados de búsqueda en un archivo de salida
void saveResults(const std::string& filepath, const std::vector<std::pair<std::string, std::set<int>>>& lookup_table) {
    std::ofstream outfile(filepath);  // Abrir archivo de salida
    if (outfile.is_open()) {
        for (const auto& pair : lookup_table) {
            if (!pair.first.empty()) {  // Asegurarse de que la palabra no esté vacía
                outfile << "(" << pair.first << ", ";  // Escribir la palabra en el archivo
                for (int doc_id : pair.second) {
                    outfile << doc_id << " ";  // Escribir cada ID de documento asociado a la palabra
                }
                outfile << ")\n";
            }
        }
    }
}

// Función para guardar los valores hash en un archivo de salida
void saveHashValues(const std::string& filepath, const std::unordered_map<std::string, unsigned long>& hash_values) {
    std::ofstream outfile(filepath);  // Abrir archivo de salida
    if (outfile.is_open()) {
        for (const auto& pair : hash_values) {
            outfile << "\"" << pair.first << "\" usando djb2 es " << pair.second << ".\n";  // Escribir la palabra y su valor hash
        }
    }
}

