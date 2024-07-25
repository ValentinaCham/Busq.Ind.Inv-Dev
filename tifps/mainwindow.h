#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <thread>
#include <mutex>
#include <windows.h>
using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked(); // Añadir Archivo
    //void on_pushButton_2_clicked(); // Buscar
    void updateFileList(); // Actualizar lista de archivos
    void processFiles(); // Procesar archivos y actualizar resultados

private:
    Ui::MainWindow *ui;
    QStringListModel *fileListModel;
    std::vector<std::string> filepaths; // Vector para rutas de archivos

    // Funciones del mapper que ya tienes
    vector<pair<int, string>> readDocuments(const vector<string> &filepaths);
    unsigned long hash_djb2(const string &word);
    void wordMapperThread(std::vector<std::pair<int, std::string>> documents,
                          std::unordered_map<std::string, std::set<int>> &intermediate_output,
                          std::unordered_map<std::string, unsigned long> &hash_values,
                          std::mutex &mtx);
    boolean isprep(string word);
    void wordMapper(vector<pair<int, string>> &mapper_output,
                    unordered_map<string, set<int>> &intermediate_output,
                    unordered_map<string, unsigned long> &hash_values);
    void saveResults(const string &filepath, const vector<pair<string, set<int>>> &lookup_table);
    void saveHashValues(const string &filepath, const unordered_map<string, unsigned long> &hash_values);
    set<int> searchWord(const string &word, const vector<pair<string, set<int>>> &lookup_table);
    vector<string> getFilesInDirectory(const string &path = ".");
    set<int> searchWordMap(const string &word, const unordered_map<string, set<int>> &lookup_table);
    set<int> searchInput(const string &word, const unordered_map<string, set<int>> &lookup_table);

};
#endif // MAINWINDOW_H
