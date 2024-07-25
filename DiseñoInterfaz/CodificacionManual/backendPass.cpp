#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <thread>
#include <mutex>
#include <windows.h>
#include <filesystem> // Librería para manejar archivos y directorios
//Para pasarlo como backend
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace std;
namespace fs = filesystem; // Alias para facilitar la escritura
// Si es que preposición o algún otro tipo de palabra que no se debe considerar
boolean isprep(string word)
{
    string preps[] = {"a", "se", "al", "ante", "del", "cabe", "con", "sino", "de", "desde", "si", "su", "en", "entre", "sus", "hasta", "mío", "para", "sí", "yó", "por", "la", "sin", "so", "sobre", "tras", "él", "no", "lo", "un", "y", "o", "ni", "que", "pero", "aunque", "sino", "si", "el", "ello", "iba", "sido", "cada", "los", "ya", "cual"};
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

// Función hash djb2
unsigned long hash_djb2(const string &word)
{
    unsigned long hash_value = 5381;
    for (char c : word)
    {
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    return hash_value;
}

// mapper para hilos
void wordMapperThread(const vector<pair<int, string>> &documents, 
                      unordered_map<string, set<int>> &intermediate_output, 
                      unordered_map<string, unsigned long> &hash_values, 
                      mutex &mtx) 
{
    for (const auto &pair : documents) 
    {
        int doc_id = pair.first;
        string document = pair.second;
        string word;

        for (char c : document) 
        {
            if (isspace(c) || ispunct(c)) 
            {
                if (!word.empty()) 
                { // termino una palabra
                    if (!isprep(word)) 
                    {
                        lock_guard<mutex> lock(mtx);
                        intermediate_output[word].insert(doc_id);
                        hash_values[word] = hash_djb2(word);
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
            lock_guard<mutex> lock(mtx);
            intermediate_output[word].insert(doc_id);
            hash_values[word] = hash_djb2(word);
        }
    }
}

void wordMapper(vector<pair<int, string>> &mapper_output, 
                unordered_map<string, set<int>> &intermediate_output, 
                unordered_map<string, unsigned long> &hash_values) 
{
    mutex mtx;
    int num_threads = thread::hardware_concurrency();
    wcout << L"num_threads: " << num_threads << endl;
    vector<thread> threads;
    int chunk_size = mapper_output.size() / num_threads;

    for (int i = 0; i < num_threads; ++i) 
    {
        auto start_itr = mapper_output.begin() + i * chunk_size;
        auto end_itr = (i == num_threads - 1) ? mapper_output.end() : start_itr + chunk_size;
        vector<pair<int, string>> chunk(start_itr, end_itr);

        threads.emplace_back(wordMapperThread, chunk, ref(intermediate_output), ref(hash_values), ref(mtx));
    }

    for (auto &t : threads) 
    {
        t.join();
    }
}

// Función para procesar archivos de texto
vector<pair<int, string>> readDocuments(const vector<string> &filepaths)
{
    // Ingresa cada documento con id y contenido en el vector de pares
    vector<pair<int, string>> documents;
    int doc_id = 1;
    for (const auto &filepath : filepaths)
    {
        ifstream file(filepath, ios::binary);
        if (file.is_open())
        {
            stringstream buffer;
            buffer << file.rdbuf();
            documents.emplace_back(doc_id++, buffer.str());
        }
    }
    return documents;
}

// Función para guardar los resultados en un archivo de salida
void saveResults(const string &filepath, const vector<pair<string, set<int>>> &lookup_table)
{
    ofstream outfile(filepath);
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
void saveHashValues(const string &filepath, const unordered_map<string, unsigned long> &hash_values)
{
    ofstream outfile(filepath);
    if (outfile.is_open())
    {
        for (const auto &pair : hash_values)
        {
            outfile << "\"" << pair.first << "\" usando djb2 es " << pair.second << ".\n";
        }
    }
}

// Función de búsqueda por índice invertido
set<int> searchWord(const string &word, const vector<pair<string, set<int>>> &lookup_table)
{
    unsigned long position = hash_djb2(word) % lookup_table.size();
    if (lookup_table[position].first == word)
    {
        return lookup_table[position].second;
    }
    return {};
}

// Método que devuelve una lista de archivos en una ruta path
vector<string> getFilesInDirectory(const string &path = ".")
{
    vector<string> archivosTxt;

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
        cerr << "Error al abrir el directorio: " << GetLastError() << endl;
    }

    return archivosTxt;
}

// Función de búsqueda por índice invertido
set<int> searchWordMap(const string &word, const unordered_map<string, set<int>> &lookup_table)
{
    auto it = lookup_table.find(word);
    if (it != lookup_table.end())
    {
        return it->second;
    }
    return {};
}

// Función serach input para operaciones en el input: e.g. "palabra1 AND palabra2 OR palabra3 NOT palabra4"
set<int> searchInput(const string &word, const unordered_map<string, set<int>> &lookup_table)
{
    // registrar las operaciones
    set<string> operations = {"AND", "OR", "NOT"};
    // arreglo de operaciones en el input
    vector<string> input_operations;
    // arreglo de palabras en el input
    vector<string> input_words;

    // separar las palabras y operaciones
    string word_temp = "";
    for (char c : word)
    {
        if (isspace(c))
        {
            if (!word_temp.empty())
            {
                string word_temp_upper = word_temp;
                transform(word_temp_upper.begin(), word_temp_upper.end(), word_temp_upper.begin(),
                           [](unsigned char c) { return toupper(c); });
                if (operations.find(word_temp_upper) != operations.end())
                {
                    input_operations.push_back(word_temp_upper);
                }
                else
                {
                    input_words.push_back(word_temp);
                }
                word_temp.clear();
            }
        }
        else
        {
            word_temp += tolower(static_cast<unsigned char>(c));
        }
    }
    // ultima palabra
    if (!word_temp.empty())
    {
        // mostrar el word_temp
        if (operations.find(word_temp) != operations.end())
        {
            input_operations.push_back(word_temp);
        }
        else
        {
            input_words.push_back(word_temp);
        }
        word_temp.clear();
    }

    // Arreglo de resultados de las palabras
    vector<set<int>> results;
    for (const string &word : input_words)
    {
        results.push_back(searchWordMap(word, lookup_table));
    }

    // Mostrar los documentos segun palabras
    for (size_t i = 0; i < input_words.size(); i++)
    {
        wcout << L"Documentos que contienen la palabra \"" << wstring(input_words[i].begin(), input_words[i].end()) << L"\": ";
        wcout << results[i].size() << L" documento(s)" << endl;
        wcout << L"Archivos ";
        for (int doc_id : results[i])
        {
            // ids
            wcout << doc_id << L" ";
        }
        wcout << endl;
    }
    // hacer operaciones con los sets de resultados en base al orden de las operaciones
    set<int> result;
    result = results[0];
    if (!input_operations.empty())
    {
        for (size_t i = 0; i < input_operations.size(); i++)
        {
            if (input_operations[i] == "AND")
            {
                wcout << L"Operación AND" << endl;
                set<int> temp;
                set_intersection(result.begin(), result.end(), results[i + 1].begin(), results[i + 1].end(), inserter(temp, temp.begin()));
                result = move(temp);
            }
            else if (input_operations[i] == "OR")
            {
                wcout << L"Operacion OR" << endl;
                set<int> temp;
                set_union(result.begin(), result.end(), results[i + 1].begin(), results[i + 1].end(), inserter(temp, temp.begin()));
                result = move(temp);
            }
            else if (input_operations[i] == "NOT")
            {
                if (!results[i + 1].empty())
                {
                    wcout << L"Operación NOT: ";
                    set<int> temp;
                    set_difference(result.begin(), result.end(), results[i + 1].begin(), results[i + 1].end(), inserter(temp, temp.begin()));
                    result = move(temp);
                }
            }
            else
            {
                wcout << L"Operación no validada" << endl;
                // print the operation
                wcout << wstring(input_operations[i].begin(), input_operations[i].end()) << endl;
            }
        }
    }
    else
    {
        wcout << L"Operación simple" << endl;
    }
    return result;
}

int main() 
{
    /*Para establecer una correcta lectura*/
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    wcin.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
    wcout.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
    /*
    vector<string> filepaths = {"file1.txt", "file2.txt", "file3.txt", "articulo.txt", "etica_profesional.txt"};

    for (const auto &filepath : filepaths) 
    {
        wcout << wstring(filepath.begin(), filepath.end()) << endl;
    }
    */

    vector<string> filepaths;
    string folderPath = "archivos"; // Carpeta donde se encuentran los archivos

    // Iterar sobre cada archivo en el directorio
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) { // Asegurarse de que la entrada es un archivo regular
            // Obtener la ruta completa del archivo y agregarla al vector
            filepaths.push_back(entry.path().string());
        }
    }

    // Imprimir las rutas de los archivos
    for (const auto& filepath : filepaths) {
        wcout << wstring(filepath.begin(), filepath.end()) << endl;
    }

    vector<pair<int, string>> mapper_output = readDocuments(filepaths);

    unordered_map<string, set<int>> intermediate_output;
    unordered_map<string, unsigned long> hash_values;

    wordMapper(mapper_output, intermediate_output, hash_values);

    const unsigned long table_size = intermediate_output.size();
    wcout << L"table_size: " << table_size << endl;
    vector<pair<string, set<int>>> lookup_table(table_size);

    for (const auto &pair : intermediate_output) 
    {
        const string &word = pair.first;
        const set<int> &doc_ids = pair.second;
        unsigned long position = hash_djb2(word) % table_size;
        if (!lookup_table[position].first.empty()) 
        {
            while (!lookup_table[position].first.empty()) 
            {
                position = (position + 1) % table_size;
            }
        }
        lookup_table[position] = make_pair(word, doc_ids);
    }

    saveResults("output.txt", lookup_table);
    saveHashValues("hash_values.txt", hash_values);

    wcout << L"Ingrese la palabra a buscar: ";
    wstring search_word;
    getline(wcin, search_word);
    wcout << L"Palabra ingresada: " << search_word << endl;

    wstring_convert<codecvt_utf8<wchar_t>> converter;
    string search_word_str = converter.to_bytes(search_word);

    set<int> result = searchInput(search_word_str, intermediate_output);

    wcout << L"Documentos que contienen la palabra \"" << search_word << L"\": ";
    wcout << result.size() << L" documento(s)" << endl;
    wcout << L"Archivos ";
    for (int doc_id : result) 
    {
        wcout << wstring(filepaths[doc_id - 1].begin(), filepaths[doc_id - 1].end()) << L" ";
    }
    wcout << endl;

    return 0;
}

// Función que guarda el archivo en la carpeta "archivos"
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void upload_file(uint8_t* data, int size, const char* filename) {
        std::string folder = "archivos";
        std::string file_path = folder + "/" + filename;

        // Crea la carpeta si no existe
        if (!fs::exists(folder)) {
            fs::create_directory(folder);
        }

        std::ofstream file(file_path, std::ios::binary);
        if (file) {
            file.write(reinterpret_cast<char*>(data), size);
            file.close();
            std::cout << "Archivo guardado exitosamente: " << file_path << std::endl;
        } else {
            std::cerr << "Error al guardar el archivo." << std::endl;
        }
    }
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("upload_file", &upload_file);
}