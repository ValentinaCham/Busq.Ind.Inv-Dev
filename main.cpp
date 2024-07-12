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

// Función de búsqueda por índice invertido
std::set<int> searchWord(const std::string& word, const std::vector<std::pair<std::string, std::set<int>>>& lookup_table) {
    unsigned long position = hash_djb2(word) % lookup_table.size();  // Calcular la posición basada en el hash de la palabra
    if (lookup_table[position].first == word) {  // Si la palabra coincide con la encontrada en esa posición
        return lookup_table[position].second;  // Devolver los IDs de documento asociados a esa palabra
    }
    return {};  // Devolver conjunto vacío si la palabra no está presente
}

int main() {
    // Lista de archivos de entrada
    std::vector<std::string> filepaths = {"file1.txt", "file2.txt", "file3.txt"};

    // Leer documentos de los archivos de entrada
    std::vector<std::pair<int, std::string>> mapper_output = readDocuments(filepaths);

    std::unordered_map<std::string, std::set<int>> intermediate_output;  // Mapa para almacenar palabras y sus IDs de documentos
    std::unordered_map<std::string, unsigned long> hash_values;  // Mapa para almacenar valores hash de palabras

    // Mapper
    for (const auto& pair : mapper_output) {
        int doc_id = pair.first;
        std::string document = pair.second;
        std::string word;
        for (char c : document) {
            if (isspace(c)) {  // Si el carácter es un espacio, se termina la palabra actual
                if (!word.empty()) {
                    intermediate_output[word].insert(doc_id);  // Insertar palabra y su ID de documento en el mapa
                    hash_values[word] = hash_djb2(word);  // Calcular y guardar el valor hash de la palabra
                    word.clear();  // Limpiar la palabra para la siguiente
                }
            } else {
                word += tolower(c);  // Convertir el carácter a minúscula y añadirlo a la palabra actual
            }
        }
        if (!word.empty()) {  // Si queda una palabra al final del documento
            intermediate_output[word].insert(doc_id);  // Insertarla en el mapa
            hash_values[word] = hash_djb2(word);  // Calcular y guardar su valor hash
        }
    }

