// Программа для поиска минимального остовного дерева
// Реализует алгоритмы Прима и Крускаля, используя только базовые средства C++
// (без STL-контейнеров типа vector, map и т.п.)

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

// Максимально возможное количество вершин и рёбер
const int MAX_VERTICES = 999;
const int MAX_EDGES = 10000;
const int INF = 999999;

// Структура для представления ребра с весом
struct Edge {
    int u;
    int v;
    int weight;
};

// Структура для хранения результата работы алгоритмов MST
struct MSTResult {
    Edge edges[MAX_VERTICES - 1];
    int edgeCount;
    int totalWeight;
    bool found;
};

// ------------------------------------------------------------
// Чтение графа из входного файла
// ------------------------------------------------------------
bool readGraph(const char* filename, int& n, int& m, Edge* edges) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open input file " << filename << std::endl;
        return false;
    }

    fin >> n >> m;
    if (n <= 0 || m < 0 || n > MAX_VERTICES || m > MAX_EDGES) {
        std::cerr << "Error: invalid number of vertices or edges" << std::endl;
        fin.close();
        return false;
    }

    for (int i = 0; i < m; ++i) {
        int u, v, w;
        if (!(fin >> u >> v >> w)) {
            std::cerr << "Error: not enough edges in file" << std::endl;
            fin.close();
            return false;
        }
        if (u < 1 || u > n || v < 1 || v > n) {
            std::cerr << "Error: vertex out of range in edge " << i + 1 << std::endl;
            fin.close();
            return false;
        }
        edges[i].u = u - 1;
        edges[i].v = v - 1;
        edges[i].weight = w;
    }

    fin.close();
    return true;
}

// ------------------------------------------------------------
// Сортировка рёбер по весу (пузырьком)
// ------------------------------------------------------------
void sortEdgesByWeight(Edge* edges, int count) {
    for (int i = 0; i < count - 1; ++i) {
        for (int j = 0; j < count - i - 1; ++j) {
            if (edges[j].weight > edges[j + 1].weight) {
                Edge temp = edges[j];
                edges[j] = edges[j + 1];
                edges[j + 1] = temp;
            }
        }
    }
}

// ------------------------------------------------------------
// Запись результата MST в выходной файл
// ------------------------------------------------------------
void writeMSTResult(std::ofstream& fout, const char* algorithmName, const MSTResult& result) {
    fout << algorithmName << ": ";
    if (result.found) {
        for (int i = 0; i < result.edgeCount; ++i) {
            fout << "(" << result.edges[i].u + 1 << "," << result.edges[i].v + 1 << ")";
            if (i < result.edgeCount - 1) fout << " ";
        }
        fout << " " << result.totalWeight;
    }
    fout << std::endl;
}

// ============================================================
// АЛГОРИТМ ПРИМА
// ============================================================
MSTResult primAlgorithm(int n, const Edge* edges, int m) {
    MSTResult result;
    result.found = false;
    result.edgeCount = 0;
    result.totalWeight = 0;

    // Матрица смежности (static — чтобы не на стеке)
    static int weight[MAX_VERTICES][MAX_VERTICES];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            weight[i][j] = -1;
        }
    }
    for (int i = 0; i < m; ++i) {
        int u = edges[i].u;
        int v = edges[i].v;
        int w = edges[i].weight;
        if (u == v) continue; // игнорируем петли
        if (weight[u][v] == -1 || w < weight[u][v]) {
            weight[u][v] = w;
            weight[v][u] = w;
        }
    }

    bool inMST[MAX_VERTICES];
    int minWeight[MAX_VERTICES];
    int parent[MAX_VERTICES];

    for (int i = 0; i < n; ++i) {
        inMST[i] = false;
        minWeight[i] = INF;
        parent[i] = -1;
    }

    minWeight[0] = 0;
    parent[0] = -1;

    for (int count = 0; count < n; ++count) {
        int u = -1;
        int minVal = INF;
        for (int i = 0; i < n; ++i) {
            if (!inMST[i] && minWeight[i] < minVal) {
                minVal = minWeight[i];
                u = i;
            }
        }

        if (u == -1) {
            std::cerr << "Prim: graph is disconnected, MST does not exist." << std::endl;
            result.found = false;
            result.edgeCount = 0;
            result.totalWeight = 0;
            return result;
        }

        inMST[u] = true;

        if (parent[u] != -1) {
            result.edges[result.edgeCount].u = parent[u];
            result.edges[result.edgeCount].v = u;
            result.edges[result.edgeCount].weight = minWeight[u];
            result.edgeCount++;
            result.totalWeight += minWeight[u];
        }

        for (int v = 0; v < n; ++v) {
            if (!inMST[v] && weight[u][v] != -1) {
                if (weight[u][v] < minWeight[v]) {
                    minWeight[v] = weight[u][v];
                    parent[v] = u;
                }
            }
        }
    }

    result.found = true;
    return result;
}

// ============================================================
// АЛГОРИТМ КРУСКАЛЯ
// ============================================================
MSTResult kruskalAlgorithm(int n, const Edge* edges, int m) {
    MSTResult result;
    result.found = false;
    result.edgeCount = 0;
    result.totalWeight = 0;

    // Копия рёбер (static — чтобы не на стеке)
    static Edge sortedEdges[MAX_EDGES];
    for (int i = 0; i < m; ++i) {
        sortedEdges[i] = edges[i];
    }

    sortEdgesByWeight(sortedEdges, m);

    int parent[MAX_VERTICES];
    int rank[MAX_VERTICES];

    for (int i = 0; i < n; ++i) {
        parent[i] = i;
        rank[i] = 0;
    }

    for (int i = 0; i < m && result.edgeCount < n - 1; ++i) {
        int u = sortedEdges[i].u;
        int v = sortedEdges[i].v;

        if (u == v) continue; // петля

        // Поиск корня с сжатием путей
        int rootU = u;
        while (parent[rootU] != rootU) {
            parent[rootU] = parent[parent[rootU]];
            rootU = parent[rootU];
        }
        int rootV = v;
        while (parent[rootV] != rootV) {
            parent[rootV] = parent[parent[rootV]];
            rootV = parent[rootV];
        }

        if (rootU != rootV) {
            result.edges[result.edgeCount] = sortedEdges[i];
            result.edgeCount++;
            result.totalWeight += sortedEdges[i].weight;

            if (rank[rootU] < rank[rootV]) {
                parent[rootU] = rootV;
            }
            else if (rank[rootU] > rank[rootV]) {
                parent[rootV] = rootU;
            }
            else {
                parent[rootV] = rootU;
                rank[rootU]++;
            }
        }
    }

    if (result.edgeCount == n - 1) {
        result.found = true;
    }
    else {
        std::cerr << "Kruskal: graph is disconnected, MST does not exist." << std::endl;
    }

    return result;
}

// ============================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    const char* inputFilename = argv[1];
    const char* outputFilename = "result.txt";

    int n, m;
    static Edge edges[MAX_EDGES];
    if (!readGraph(inputFilename, n, m, edges)) {
        return 1;
    }

    MSTResult primResult = primAlgorithm(n, edges, m);
    MSTResult kruskalResult = kruskalAlgorithm(n, edges, m);

    std::ofstream fout(outputFilename);
    if (!fout.is_open()) {
        std::cerr << "Error: cannot open output file " << outputFilename << std::endl;
        return 1;
    }

    writeMSTResult(fout, "Prim's algorithm", primResult);
    writeMSTResult(fout, "Kruskal's algorithm", kruskalResult);

    fout.close();

    if (!primResult.found) {
        std::cerr << "Prim's algorithm: MST not found (graph is disconnected)." << std::endl;
    }
    if (!kruskalResult.found) {
        std::cerr << "Kruskal's algorithm: MST not found (graph is disconnected)." << std::endl;
    }

    // Запуск выходного файла (Windows-only)
    system("result.txt");

    return 0;
}
