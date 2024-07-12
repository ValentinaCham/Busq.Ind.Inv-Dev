#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <windows.h>
// Función hash djb2
unsigned long hash_djb2(const std::string &word)
{
    unsigned long hash_value = 5381;
    for (char c : word)
    {
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    return hash_value;
}

// Función para procesar archivos de texto
std::vector<std::pair<int, std::string>> readDocuments(const std::vector<std::string> &filepaths)
{
    // Ingresa cada documento con id y contenido en el vector de pares
    std::vector<std::pair<int, std::string>> documents;
    int doc_id = 1;
    for (const auto &filepath : filepaths)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            documents.emplace_back(doc_id++, buffer.str());
        }
    }
    return documents;
}

// Función para guardar los resultados en un archivo de salida
void saveResults(const std::string &filepath, const std::vector<std::pair<std::string, std::set<int>>> &lookup_table)
{
    std::ofstream outfile(filepath);
    if (outfile.is_open())
    {
        for (const auto &pair : lookup_table)
        {
            if (!pair.first.empty())
            {
                outfile << "(" << pair.first << ", ";
                for (int doc_id : pair.second)
                {
                    outfile << doc_id << " ";
                }
                outfile << ")\n";
            }
        }
    }
}

// Función para guardar los valores hash en un archivo de salida
void saveHashValues(const std::string &filepath, const std::unordered_map<std::string, unsigned long> &hash_values)
{
    std::ofstream outfile(filepath);
    if (outfile.is_open())
    {
        for (const auto &pair : hash_values)
        {
            outfile << "\"" << pair.first << "\" usando djb2 es " << pair.second << ".\n";
        }
    }
}

// Función de búsqueda por índice invertido
std::set<int> searchWord(const std::string &word, const std::vector<std::pair<std::string, std::set<int>>> &lookup_table)
{
    unsigned long position = hash_djb2(word) % lookup_table.size();
    if (lookup_table[position].first == word)
    {
        return lookup_table[position].second;
    }
    return {};
}

// Si es que preposición o algún otro tipo de palabra que no se debe considerar
boolean isprep(std::string word)
{
    std::string preps[] = {"a","se", "al", "ante", "del", "cabe", "con", "sino", "de", "desde", "si", "su", "en", "entre", "sus", "hasta", "mío", "para", "sí", "yó", "por", "la", "sin", "so", "sobre", "tras", "él", "no", "lo", "un", "y", "o", "ni", "que", "pero", "aunque", "sino", "si", "el", "ello", "iba", "sido", "cada", "los", "ya", "cual"};
    // size of preps
    int size = sizeof(preps) / sizeof(preps[0]);
    for (int i = 0; i < size; i++)
    {
        if (word == preps[i] || word.length() == 1)
        {
            return true;
        }
    }
    return false;
}
// Método que devuelve una lista de archivos en una ruta path
std::vector<std::string> getFilesInDirectory(const std::string &path = ".")
{
    std::vector<std::string> archivosTxt;

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((path + "\\*.txt").c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            archivosTxt.push_back(findFileData.cFileName);
        } while (FindNextFileA(hFind, &findFileData));

        FindClose(hFind);
    }
    else
    {
        // Error al abrir el path
        std::cerr << "Error al abrir el directorio: " << GetLastError() << std::endl;
    }

    return archivosTxt;
}

// Función de búsqueda por índice invertido
std::set<int> searchWordMap(const std::string &word, const std::unordered_map<std::string, std::set<int>> &lookup_table)
{
    auto it = lookup_table.find(word);
    if (it != lookup_table.end())
    {
        return it->second;
    }
    return {};
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Configurar wcin y wcout para trabajar con UTF-8
    std::wcin.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    std::wcout.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));

    // Lista de archivos de entrada
    std::vector<std::string> filepaths = {"file1.txt", "file2.txt", "file3.txt"};
    // std::vector<std::string> filepaths = getFilesInDirectory("./txt");
    // print filepaths
    // for (int i = 0; i < filepaths.size(); i++)
    // {  
    //     std::wcout << std::wstring(filepaths[i].begin(), filepaths[i].end()) << std::endl;
    // }

    // Leer documentos de los archivos de entrada
    std::vector<std::pair<int, std::string>> mapper_output = readDocuments(filepaths);

    std::unordered_map<std::string, std::set<int>> intermediate_output;
    std::unordered_map<std::string, unsigned long> hash_values;

        std::wcout << L"Archivos de entrada: " << std::endl;
    // Mapper - Cada palabra será asignada a una lista de archivos
    for (const auto &pair : mapper_output)
    {
        int doc_id = pair.first;
        std::string document = pair.second;
        std::string word;
        for (char c : document)
        {
            if (isspace(c) || ispunct(c))
            {
                if (!word.empty())
                { // termino una palabra
                    if (!isprep(word))
                    {
                        intermediate_output[word].insert(doc_id);
                        hash_values[word] = hash_djb2(word);
                        // std::wcout << "Palabra aceptada: " << std::wstring(word.begin(), word.end()) << std::endl;
                    }
                    word.clear();
                    
                }
            }
            else
            {
                word += tolower(static_cast<unsigned char>(c));
            }
        }
        if (!word.empty() && !isprep(word))
        {
            intermediate_output[word].insert(doc_id);
            hash_values[word] = hash_djb2(word);
            // std::wcout << "Palabra aceptada:" << std::wstring(word.begin(), word.end()) << std::endl;
        }
    }

    // Reducer
    const unsigned long table_size = intermediate_output.size();
    // print size
    std::wcout << L"table_size: " << table_size << std::endl;
    std::vector<std::pair<std::string, std::set<int>>> lookup_table(table_size);

    for (const auto &pair : intermediate_output)
    {
        const std::string &word = pair.first;
        const std::set<int> &doc_ids = pair.second;
        unsigned long position = hash_djb2(word) % table_size;
        if (!lookup_table[position].first.empty())
        {
            // Resolver colisiones con encadenamiento
            while (!lookup_table[position].first.empty())
            {
                position = (position + 1) % table_size;
            }
        }
        lookup_table[position] = std::make_pair(word, doc_ids);
    }

    // Guardar los resultados en un archivo de salida
    saveResults("output.txt", lookup_table);
    // Guardar los valores hash en un archivo separado
    saveHashValues("hash_values.txt", hash_values);

    // Solicitar al usuario que ingrese una palabra para buscar
    std::wcout << L"Ingrese la palabra a buscar: ";
    std::wstring search_word;
    std::getline(std::wcin, search_word);
    std::wcout << L"Palabra ingresada: " << search_word << std::endl;
    // Convertir la palabra de búsqueda a std::string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string search_word_str = converter.to_bytes(search_word);

    // Búsqueda con vector y entero
    // std::set<int> result = searchWord(search_word_str, lookup_table);
    std::set<int> result = searchWordMap(search_word_str, intermediate_output);

    // Mostrar resultados de la búsqueda
    std::wcout << L"Documentos que contienen la palabra \"" << search_word << L"\": ";
    std::wcout << result.size() << L" documento(s)" << std::endl;
    std::wcout << L"Archivos ";
    for (int doc_id : result)
    {
        std::wcout << std::wstring(filepaths[doc_id - 1].begin(), filepaths[doc_id - 1].end()) << L" ";
    }
    std::wcout << std::endl;

    return 0;
}