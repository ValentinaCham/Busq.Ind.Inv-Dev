#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDebug>
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
#include <filesystem>
using namespace std;
namespace fs = filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fileListModel(new QStringListModel(this))
{
    ui->setupUi(this);

    // Conectar botones a sus slots
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);
    //connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::on_pushButton_2_clicked);

    // Asignar el modelo a listView_2
    ui->listView_2->setModel(fileListModel);

    // Actualizar la lista de archivos al inicio
    updateFileList();
    processFiles(); // Procesar archivos al inicio
}
/*
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fileListModel(new QStringListModel(this)) // Inicialización del modelo
{
    ui->setupUi(this);

    // Conectar el botón al slot
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);

    // Asignar el modelo a listView_2
    ui->listView_2->setModel(fileListModel);

    // Actualizar la lista de archivos al inicio
    updateFileList();
}*/

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // Open a file dialog for the user to select a file
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select File"), QDir::homePath());

    // Check if the user selected a file
    if (!filePath.isEmpty()) {
        // Extract the file name
        QString fileName = QFileInfo(filePath).fileName();

        // Define the destination directory
        QString destinationDir = "archivos";

        // Ensure the directory exists, create it if necessary
        QDir dir;
        if (!dir.exists(destinationDir)) {
            if (!dir.mkdir(destinationDir)) {
                QMessageBox::warning(this, tr("Error"), tr("No se pudo crear el directorio de destino."));
                return;
            }
        }

        // Construct the destination file path
        QString destinationFilePath = QDir(destinationDir).filePath(fileName);

        // Copy the file to the destination directory
        if (QFile::copy(filePath, destinationFilePath)) {
            QMessageBox::information(this, tr("Success"), tr("Archivo guardado exitosamente en: ") + destinationFilePath);
            qDebug() << "Archivo guardado en:" << destinationFilePath;
            updateFileList();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar el archivo."));
            qDebug() << "Error al guardar el archivo en:" << destinationFilePath;
        }
    }
}

void MainWindow::updateFileList()
{
    // Definir el directorio de destino
    QString directoryPath = "archivos";

    // Crear un objeto QDir con el directorio de destino
    QDir dir(directoryPath);

    // Verificar si el directorio existe
    if (!dir.exists()) {
        qDebug() << "El directorio no existe:" << directoryPath;
        return;
    }

    // Obtener la lista de archivos en el directorio
    QStringList fileList = dir.entryList(QDir::Files);

    // Asignar la lista de archivos al modelo
    fileListModel->setStringList(fileList);

    // Actualizar filepaths para procesar archivos
    filepaths.clear();
    for (const QString &fileName : fileList) {
        QString filePath = dir.filePath(fileName);
        filepaths.push_back(filePath.toStdString());
    }

    // Salida de depuración
    qDebug() << "Lista de archivos actualizada:" << fileList;
}

void MainWindow::processFiles()
{
    if (filepaths.empty()) {
        qDebug() << "No hay archivos para procesar.";
        return;
    }

    // Imprimir las rutas de los archivos
    for (const auto& filepath : filepaths) {
        std::wcout << std::wstring(filepath.begin(), filepath.end()) << std::endl;
    }

    std::vector<std::pair<int, std::string>> mapper_output = readDocuments(filepaths);

    std::unordered_map<std::string, std::set<int>> intermediate_output;
    std::unordered_map<std::string, unsigned long> hash_values;

    wordMapper(mapper_output, intermediate_output, hash_values);

    const unsigned long table_size = intermediate_output.size();
    std::wcout << L"table_size: " << table_size << std::endl;
    std::vector<std::pair<std::string, std::set<int>>> lookup_table(table_size);

    for (const auto& pair : intermediate_output) {
        const std::string& word = pair.first;
        const std::set<int>& doc_ids = pair.second;
        unsigned long position = hash_djb2(word) % table_size;
        if (!lookup_table[position].first.empty()) {
            while (!lookup_table[position].first.empty()) {
                position = (position + 1) % table_size;
            }
        }
        lookup_table[position] = std::make_pair(word, doc_ids);
    }

    saveResults("output.txt", lookup_table);

    qDebug() << "Archivos procesados y resultados guardados en output.txt.";
}

boolean MainWindow::isprep(string word)
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
unsigned long MainWindow::hash_djb2(const string &word)
{
    unsigned long hash_value = 5381;
    for (char c : word)
    {
        hash_value = ((hash_value << 5) + hash_value) + c;
    }
    return hash_value;
}

void MainWindow::wordMapper(std::vector<std::pair<int, std::string>> &mapper_output,
                            std::unordered_map<std::string, std::set<int>> &intermediate_output,
                            std::unordered_map<std::string, unsigned long> &hash_values)
{
    std::mutex mtx;
    int num_threads = std::thread::hardware_concurrency();
    std::wcout << L"num_threads: " << num_threads << std::endl;
    std::vector<std::thread> threads;
    int chunk_size = mapper_output.size() / num_threads;

    for (int i = 0; i < num_threads; ++i)
    {
        auto start_itr = mapper_output.begin() + i * chunk_size;
        auto end_itr = (i == num_threads - 1) ? mapper_output.end() : start_itr + chunk_size;
        std::vector<std::pair<int, std::string>> chunk(start_itr, end_itr);

        // Lanzar el hilo y pasar la función como un puntero a miembro
        threads.emplace_back(&MainWindow::wordMapperThread, this, chunk,
                             std::ref(intermediate_output), std::ref(hash_values), std::ref(mtx));
    }

    for (auto &t : threads)
    {
        t.join();
    }
}

void MainWindow::wordMapperThread(std::vector<std::pair<int, std::string>> documents,
                                  std::unordered_map<std::string, std::set<int>> &intermediate_output,
                                  std::unordered_map<std::string, unsigned long> &hash_values,
                                  std::mutex &mtx)
{
    for (const auto &pair : documents)
    {
        int doc_id = pair.first;
        std::string document = pair.second;
        std::string word;

        for (char c : document)
        {
            if (std::isspace(c) || std::ispunct(c))
            {
                if (!word.empty())
                { // termino una palabra
                    if (!isprep(word))
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        intermediate_output[word].insert(doc_id);
                        hash_values[word] = hash_djb2(word);
                    }
                    word.clear();
                }
            }
            else
            {
                word += std::tolower(static_cast<unsigned char>(c));
            }
        }

        if (!word.empty() && !isprep(word))
        {
            std::lock_guard<std::mutex> lock(mtx);
            intermediate_output[word].insert(doc_id);
            hash_values[word] = hash_djb2(word);
        }
    }
}


// Función para procesar archivos de texto
vector<pair<int, string>> MainWindow::readDocuments(const vector<string> &filepaths)
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
void MainWindow::saveResults(const string &filepath, const vector<pair<string, set<int>>> &lookup_table)
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
void MainWindow::saveHashValues(const string &filepath, const unordered_map<string, unsigned long> &hash_values)
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
set<int> MainWindow::searchWord(const string &word, const vector<pair<string, set<int>>> &lookup_table)
{
    unsigned long position = hash_djb2(word) % lookup_table.size();
    if (lookup_table[position].first == word)
    {
        return lookup_table[position].second;
    }
    return {};
}

// Método que devuelve una lista de archivos en una ruta path
vector<string> MainWindow::getFilesInDirectory(const string &path = ".")
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
set<int> MainWindow::searchWordMap(const string &word, const unordered_map<string, set<int>> &lookup_table)
{
    auto it = lookup_table.find(word);
    if (it != lookup_table.end())
    {
        return it->second;
    }
    return {};
}

// Función serach input para operaciones en el input: e.g. "palabra1 AND palabra2 OR palabra3 NOT palabra4"
set<int> MainWindow::searchInput(const string &word, const unordered_map<string, set<int>> &lookup_table)
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

