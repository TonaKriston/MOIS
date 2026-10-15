#include <iostream>
#include <fstream>
#include <climits>
#include <cstdlib>

using namespace std;

// ------------------------------------------------------------
// Структура для хранения одного ребра в списке смежности
// ------------------------------------------------------------
struct Edge {
    int to;       // номер вершины, в которую ведёт ребро
    int weight;   // вес ребра
    Edge* next;   // указатель на следующее ребро в списке
};

// ------------------------------------------------------------
// Функция добавления ребра в список смежности
// head - массив головных указателей списков смежности
// from - откуда, to - куда, weight - вес
// ------------------------------------------------------------
void addEdge(Edge** head, int from, int to, int weight) {
    Edge* newEdge = new Edge;
    newEdge->to = to;
    newEdge->weight = weight;
    newEdge->next = head[from];
    head[from] = newEdge;
}

// ------------------------------------------------------------
// Алгоритм Дейкстры
// graph - массив списков смежности
// vertexCount - количество вершин
// source - номер начальной вершины (1..vertexCount)
// distance - массив для кратчайших расстояний (размер vertexCount+1)
// previous - массив предшественников (размер vertexCount+1)
// reachable - массив достижимости (размер vertexCount+1)
// ------------------------------------------------------------
void Dijkstra(Edge** graph, int vertexCount, int source,
    int* distance, int* previous, bool* reachable) {
    // Инициализация
    for (int i = 1; i <= vertexCount; i++) {
        distance[i] = INT_MAX;
        previous[i] = 0;
        reachable[i] = false;
    }
    distance[source] = 0;

    // Массив посещённых вершин
    bool* visited = new bool[vertexCount + 1];
    for (int i = 1; i <= vertexCount; i++) {
        visited[i] = false;
    }

    // Основной цикл: повторяем vertexCount раз
    for (int step = 0; step < vertexCount; step++) {
        // Находим непосещённую вершину с минимальным расстоянием
        int minDistance = INT_MAX;
        int currentVertex = -1;
        for (int i = 1; i <= vertexCount; i++) {
            if (!visited[i] && distance[i] < minDistance) {
                minDistance = distance[i];
                currentVertex = i;
            }
        }

        // Если не нашли вершину (все оставшиеся недостижимы) — выходим
        if (currentVertex == -1) {
            break;
        }

        visited[currentVertex] = true;
        reachable[currentVertex] = true;

        // Просматриваем всех соседей текущей вершины
        Edge* edge = graph[currentVertex];
        while (edge != NULL) {
            int neighbor = edge->to;
            // Проверяем, чтобы не было переполнения
            if (distance[currentVertex] != INT_MAX) {
                int newDistance = distance[currentVertex] + edge->weight;
                if (newDistance < distance[neighbor]) {
                    distance[neighbor] = newDistance;
                    previous[neighbor] = currentVertex;
                }
            }
            edge = edge->next;
        }
    }

    delete[] visited;
}

// ------------------------------------------------------------
// Алгоритм Флойда-Уоршелла
// weightMatrix - матрица весов (размер (vertexCount+1) x (vertexCount+1))
// vertexCount - количество вершин
// nextMatrix - матрица для восстановления пути
// ------------------------------------------------------------
void FloydWarshall(int** weightMatrix, int vertexCount, int** nextMatrix) {
    // Инициализация nextMatrix
    for (int i = 1; i <= vertexCount; i++) {
        for (int j = 1; j <= vertexCount; j++) {
            if (i == j) {
                nextMatrix[i][j] = i;
            }
            else if (weightMatrix[i][j] != INT_MAX) {
                nextMatrix[i][j] = j;
            }
            else {
                nextMatrix[i][j] = 0;
            }
        }
    }

    // Основной цикл Флойда-Уоршелла
    for (int k = 1; k <= vertexCount; k++) {
        for (int i = 1; i <= vertexCount; i++) {
            for (int j = 1; j <= vertexCount; j++) {
                // Проверяем, можно ли улучшить путь через вершину k
                if (weightMatrix[i][k] != INT_MAX &&
                    weightMatrix[k][j] != INT_MAX &&
                    weightMatrix[i][k] + weightMatrix[k][j] < weightMatrix[i][j]) {
                    weightMatrix[i][j] = weightMatrix[i][k] + weightMatrix[k][j];
                    nextMatrix[i][j] = nextMatrix[i][k];
                }
            }
        }
    }
}

// ------------------------------------------------------------
// Восстановление пути по массиву предшественников (для Дейкстры)
// ------------------------------------------------------------
int restorePathDijkstra(int* previous, int source, int target, int* path) {
    int length = 0;
    int current = target;
    while (current != 0) {
        path[length] = current;
        length++;
        if (current == source) {
            break;
        }
        current = previous[current];
    }
    // Переворачиваем путь
    for (int i = 0; i < length / 2; i++) {
        int temp = path[i];
        path[i] = path[length - 1 - i];
        path[length - 1 - i] = temp;
    }
    return length;
}

// ------------------------------------------------------------
// Восстановление пути по матрице next (для Флойда-Уоршелла)
// ------------------------------------------------------------
int restorePathFloyd(int** nextMatrix, int source, int target, int* path) {
    if (nextMatrix[source][target] == 0) {
        return 0; // путь не существует
    }
    int length = 0;
    int current = source;
    while (current != target) {
        path[length] = current;
        length++;
        current = nextMatrix[current][target];
    }
    path[length] = target;
    length++;
    return length;
}

// ------------------------------------------------------------
// Главная функция
// ------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Проверяем, что передан аргумент командной строки
    if (argc < 2) {
        cout << "Wrong call. Use this format: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // Открываем входной файл
    ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        cout << "Can't open file " << argv[1] << endl;
        return 1;
    }

    int vertexCount;
    int edgeCount;
    inputFile >> vertexCount;
    inputFile >> edgeCount;

    if (vertexCount <= 0) {
        cout << "Impossible graph. Correct the input file" << endl;
        inputFile.close();
        return 1;
    }

    // Создаём список смежности
    Edge** graph = new Edge * [vertexCount + 1];
    for (int i = 1; i <= vertexCount; i++) {
        graph[i] = NULL;
    }

    // Создаём матрицу весов для Флойда-Уоршелла
    int** weightMatrix = new int* [vertexCount + 1];
    for (int i = 1; i <= vertexCount; i++) {
        weightMatrix[i] = new int[vertexCount + 1];
        for (int j = 1; j <= vertexCount; j++) {
            if (i == j) {
                weightMatrix[i][j] = 0;
            }
            else {
                weightMatrix[i][j] = INT_MAX;
            }
        }
    }

    // Флаг наличия отрицательных весов
    bool hasNegativeWeight = false;

    // Читаем рёбра (неориентированные — добавляем в оба направления)
    for (int i = 0; i < edgeCount; i++) {
        int from, to, weight;
        inputFile >> from >> to >> weight;

        // Проверяем на отрицательный вес
        if (weight < 0) {
            hasNegativeWeight = true;
        }

        // Добавляем в список смежности в обоих направлениях
        addEdge(graph, from, to, weight);
        addEdge(graph, to, from, weight);

        // Добавляем в матрицу весов (для неориентированного графа — симметрично)
        weightMatrix[from][to] = weight;
        weightMatrix[to][from] = weight;
    }
    inputFile.close();

    // Источник — вершина 1 (по заданию)
    int source = 1;

    // ---------------- Алгоритм Дейкстры ----------------
    int* distanceDijkstra = new int[vertexCount + 1];
    int* previousDijkstra = new int[vertexCount + 1];
    bool* reachableDijkstra = new bool[vertexCount + 1];

    if (!hasNegativeWeight) {
        Dijkstra(graph, vertexCount, source, distanceDijkstra, previousDijkstra, reachableDijkstra);
    }

    // ---------------- Алгоритм Флойда-Уоршелла ----------------
    int** nextMatrix = new int* [vertexCount + 1];
    for (int i = 1; i <= vertexCount; i++) {
        nextMatrix[i] = new int[vertexCount + 1];
    }
    FloydWarshall(weightMatrix, vertexCount, nextMatrix);

    // ---------------- Запись результатов ----------------
    ofstream outputFile("result.txt");
    if (!outputFile.is_open()) {
        cout << "Can't open output file" << endl;
        return 1;
    }

    // Вывод результатов Дейкстры
    outputFile << "Dijkstra:" << endl;
    if (hasNegativeWeight) {
        outputFile << "Can't apply Dijkstra because of negative weights" << endl;
    }
    else {
        for (int target = 1; target <= vertexCount; target++) {
            if (target == source) {
                continue; // пропускаем путь до самой себя
            }
            if (!reachableDijkstra[target] || distanceDijkstra[target] == INT_MAX) {
                outputFile << "Path to " << target << ": can't reach" << endl;
            }
            else {
                int* path = new int[vertexCount + 1];
                int pathLength = restorePathDijkstra(previousDijkstra, source, target, path);
                outputFile << "Path to " << target << ": ";
                for (int i = 0; i < pathLength; i++) {
                    outputFile << path[i];
                    if (i < pathLength - 1) {
                        outputFile << " ";
                    }
                }
                outputFile << ", total weight = " << distanceDijkstra[target] << endl;
                delete[] path;
            }
        }
    }

    // Вывод результатов Флойда-Уоршелла
    outputFile << endl << "Floyd Warshall:" << endl;
    for (int target = 1; target <= vertexCount; target++) {
        if (target == source) {
            continue;
        }
        if (weightMatrix[source][target] == INT_MAX) {
            outputFile << "Path to " << target << ": can't reach" << endl;
        }
        else {
            int* path = new int[vertexCount + 1];
            int pathLength = restorePathFloyd(nextMatrix, source, target, path);
            outputFile << "Path to " << target << ": ";
            for (int i = 0; i < pathLength; i++) {
                outputFile << path[i];
                if (i < pathLength - 1) {
                    outputFile << " ";
                }
            }
            outputFile << ", total weight = " << weightMatrix[source][target] << endl;
            delete[] path;
        }
    }

    outputFile.close();

    // ---------------- Освобождение памяти ----------------
    // Удаляем список смежности
    for (int i = 1; i <= vertexCount; i++) {
        Edge* edge = graph[i];
        while (edge != NULL) {
            Edge* temp = edge;
            edge = edge->next;
            delete temp;
        }
    }
    delete[] graph;

    // Удаляем матрицу весов
    for (int i = 1; i <= vertexCount; i++) {
        delete[] weightMatrix[i];
    }
    delete[] weightMatrix;

    // Удаляем матрицу next
    for (int i = 1; i <= vertexCount; i++) {
        delete[] nextMatrix[i];
    }
    delete[] nextMatrix;

    // Удаляем массивы Дейкстры
    delete[] distanceDijkstra;
    delete[] previousDijkstra;
    delete[] reachableDijkstra;

    cout << "Everyting is done" << endl;
    system("result.txt");

    return 0;
}