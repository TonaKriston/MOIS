// Лабораторная работа: компоненты сильной связности орграфа
// и компоненты двусвязности неориентированного графа.
//
// Программа запускается из командной строки:
//     program <input_file>
// Результат записывается в файл result.txt.
//
// Входной файл имеет следующий формат:
//   строка 1: число вершин N (вершины нумеруются 0..N-1)
//   строка 2: число дуг/рёбер M
//   далее M строк: "u v" — дуга (u -> v) для орграфа,
//                  ребро {u, v} для неориентированного графа.
// Направления дуг учитываются при поиске КСС и игнорируются
// (ребро трактуется как неориентированное) при поиске КДС.

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

using namespace std;

// ---------------------------------------------------------------
// Вспомогательная структура: динамический массив целых чисел.
// Используется как стек/очередь и как список.
// ---------------------------------------------------------------
struct IntArray {
    int* data;
    int  size;
    int  cap;

    IntArray() {
        data = 0;
        size = 0;
        cap = 0;
    }
    ~IntArray() {
        delete[] data;
    }
    void reserve(int n) {
        if (n <= cap) return;
        int* nd = new int[n];
        for (int i = 0; i < size; ++i) nd[i] = data[i];
        delete[] data;
        data = nd;
        cap = n;
    }
    void push(int x) {
        if (size == cap) reserve(cap == 0 ? 4 : cap * 2);
        data[size++] = x;
    }
    int  pop() {
        return data[--size];
    }
    int  top() const {
        return data[size - 1];
    }
    bool empty() const {
        return size == 0;
    }
    int& operator[](int i) { return data[i]; }
    const int& operator[](int i) const { return data[i]; }
};

// ---------------------------------------------------------------
// Чтение графа из файла.
// Возвращает true при успехе. Массивы from[] и to[] содержат M
// пар — концы дуг (u -> v).
// ---------------------------------------------------------------
bool readGraph(const char* filename, int& n, int& m,
    int*& from, int*& to)
{
    ifstream in(filename);
    if (!in) {
        cerr << "Cannot open input file: " << filename << "\n";
        return false;
    }
    if (!(in >> n)) {
        cerr << "Cannot read number of vertices\n";
        return false;
    }
    if (!(in >> m)) {
        cerr << "Cannot read number of edges\n";
        return false;
    }
    if (n < 0 || m < 0) {
        cerr << "Invalid sizes\n";
        return false;
    }
    from = new int[m > 0 ? m : 1];
    to = new int[m > 0 ? m : 1];
    for (int i = 0; i < m; ++i) {
        if (!(in >> from[i] >> to[i])) {
            cerr << "Cannot read edge #" << i << "\n";
            return false;
        }
        if (from[i] < 0 || from[i] >= n ||
            to[i] < 0 || to[i] >= n) {
            cerr << "Edge endpoints out of range\n";
            return false;
        }
    }
    in.close();
    return true;
}

// ---------------------------------------------------------------
// Построение списков смежности (CSR-формат):
//   offset[v] .. offset[v+1]-1  — диапазон соседей вершины v
//                                 в массиве adjList[].
// ---------------------------------------------------------------
void buildAdjacency(int n, int m, const int* from, const int* to,
    int*& offset, int*& adjList)
{
    int* cnt = new int[n];
    for (int i = 0; i < n; ++i) cnt[i] = 0;
    for (int i = 0; i < m; ++i) cnt[from[i]]++;

    offset = new int[n + 1];
    offset[0] = 0;
    for (int i = 0; i < n; ++i) offset[i + 1] = offset[i] + cnt[i];

    adjList = new int[m > 0 ? m : 1];
    int* pos = new int[n];
    for (int i = 0; i < n; ++i) pos[i] = offset[i];
    for (int i = 0; i < m; ++i) {
        int u = from[i];
        adjList[pos[u]++] = to[i];
    }
    delete[] pos;
    delete[] cnt;
}

// ---------------------------------------------------------------
// Компоненты сильной связности — алгоритм Косараю.
// Два прохода:
//   1) DFS по исходному графу, запоминаем порядок выхода (post-order);
//   2) DFS по обратному графу в обратном порядке выхода.
// ---------------------------------------------------------------
int stronglyConnectedComponents(int n, int m,
    const int* from, const int* to,
    IntArray*& comps, int*& compSizes)
{
    int* offF, * adjF;
    buildAdjacency(n, m, from, to, offF, adjF);

    int* offR, * adjR;
    buildAdjacency(n, m, to, from, offR, adjR);

    // --- Проход 1 ---
    bool* visited = new bool[n];
    for (int i = 0; i < n; ++i) visited[i] = false;

    IntArray order;
    order.reserve(n);

    IntArray stack;
    IntArray iter;
    stack.reserve(n);
    iter.reserve(n);

    for (int start = 0; start < n; ++start) {
        if (visited[start]) continue;

        stack.push(start);
        iter.push(offF[start]);
        visited[start] = true;

        while (!stack.empty()) {
            int v = stack.top();
            int& it = iter[iter.size - 1];

            if (it < offF[v + 1]) {
                int u = adjF[it++];
                if (!visited[u]) {
                    visited[u] = true;
                    stack.push(u);
                    iter.push(offF[u]);
                }
            }
            else {
                order.push(v);
                stack.pop();
                iter.pop();
            }
        }
    }

    delete[] visited;

    // --- Проход 2 ---
    bool* used = new bool[n];
    for (int i = 0; i < n; ++i) used[i] = false;

    comps = new IntArray[n];
    compSizes = new int[n];
    int nComp = 0;

    for (int k = order.size - 1; k >= 0; --k) {
        int start = order[k];
        if (used[start]) continue;

        IntArray& c = comps[nComp];
        c.reserve(4);
        c.push(start);
        used[start] = true;

        IntArray st;
        st.reserve(4);
        st.push(start);

        while (!st.empty()) {
            int v = st.pop();
            for (int p = offR[v]; p < offR[v + 1]; ++p) {
                int u = adjR[p];
                if (!used[u]) {
                    used[u] = true;
                    c.push(u);
                    st.push(u);
                }
            }
        }
        compSizes[nComp] = c.size;
        nComp++;
    }

    delete[] used;
    delete[] offF; delete[] adjF;
    delete[] offR; delete[] adjR;

    return nComp;
}

// ---------------------------------------------------------------
// Компоненты двусвязности (блоки) неориентированного графа.
// Алгоритм Тарьяна, итеративная реализация DFS.
//
// Возвращает:
//   comps         — массив списков ВЕРШИН каждого блока
//                   (в порядке первого появления при разборе рёбер);
//   compSizes     — число вершин в каждом блоке;
//   compEdges     — массив списков РЁБЕР каждого блока, уложенных
//                   подряд парами: u0, v0, u1, v1, ...
//                   (ориентация — как во входном файле);
//   compEdgeCount — число рёбер в каждом блоке
//                   (= compEdges[i].size / 2).
// ---------------------------------------------------------------
int biconnectedComponents(int n, int m,
    const int* from, const int* to,
    IntArray*& comps, int*& compSizes,
    IntArray*& compEdges, int*& compEdgeCount)
{
    // Построим неориентированный CSR: каждое ребро входит дважды.
    int m2 = 2 * m;
    int* eFrom = new int[m2 > 0 ? m2 : 1];
    int* eTo = new int[m2 > 0 ? m2 : 1];
    int* eId = new int[m2 > 0 ? m2 : 1];
    for (int i = 0; i < m; ++i) {
        eFrom[2 * i] = from[i];
        eTo[2 * i] = to[i];
        eId[2 * i] = i;
        eFrom[2 * i + 1] = to[i];
        eTo[2 * i + 1] = from[i];
        eId[2 * i + 1] = i;
    }
    int* off, * adjTo, * adjId;
    {
        int* cnt = new int[n];
        for (int i = 0; i < n; ++i) cnt[i] = 0;
        for (int i = 0; i < m2; ++i) cnt[eFrom[i]]++;

        off = new int[n + 1];
        off[0] = 0;
        for (int i = 0; i < n; ++i) off[i + 1] = off[i] + cnt[i];

        adjTo = new int[m2 > 0 ? m2 : 1];
        adjId = new int[m2 > 0 ? m2 : 1];
        int* pos = new int[n];
        for (int i = 0; i < n; ++i) pos[i] = off[i];
        for (int i = 0; i < m2; ++i) {
            int u = eFrom[i];
            adjTo[pos[u]] = eTo[i];
            adjId[pos[u]] = eId[i];
            pos[u]++;
        }
        delete[] pos;
        delete[] cnt;
    }
    delete[] eFrom; delete[] eTo; delete[] eId;

    int* tin = new int[n];
    int* low = new int[n];
    int* parent = new int[n];
    for (int i = 0; i < n; ++i) {
        tin[i] = -1;
        low[i] = -1;
        parent[i] = -1;
    }

    // Стек рёбер (по id) — для накопления блоков.
    IntArray edgeStack;
    edgeStack.reserve(16);

    comps = new IntArray[n];
    compEdges = new IntArray[n];
    compSizes = new int[n];
    compEdgeCount = new int[n];
    int nComp = 0;

    int timer = 0;

    IntArray vertexStack;
    IntArray iterStack;
    IntArray parentEdge;
    vertexStack.reserve(n);
    iterStack.reserve(n);
    parentEdge.reserve(n);

    for (int start = 0; start < n; ++start) {
        if (tin[start] != -1) continue;

        tin[start] = low[start] = timer++;
        vertexStack.push(start);
        iterStack.push(off[start]);
        parentEdge.push(-1);

        while (!vertexStack.empty()) {
            int v = vertexStack.top();
            int& it = iterStack[iterStack.size - 1];

            if (it < off[v + 1]) {
                int u = adjTo[it];
                int id = adjId[it];
                it++;

                // Пропускаем конкретное ребро, по которому пришли
                // в v (в обратную сторону). Проверка по id корректно
                // обрабатывает и кратные рёбра.
                if (parent[v] == u &&
                    parentEdge[parentEdge.size - 1] == id)
                    continue;

                if (tin[u] == -1) {
                    // Ребро дерева.
                    parent[u] = v;
                    tin[u] = low[u] = timer++;

                    edgeStack.push(id);

                    vertexStack.push(u);
                    iterStack.push(off[u]);
                    parentEdge.push(id);
                }
                else {
                    // Обратное ребро (u — предок или уже посещён).
                    if (tin[u] < low[v]) low[v] = tin[u];
                    // Кладём в стек только один раз — «снизу вверх»,
                    // т.е. когда tin[u] < tin[v] (u — предок v).
                    if (tin[u] < tin[v])
                        edgeStack.push(id);
                }
            }
            else {
                // Все соседи обработаны — выходим из v.
                vertexStack.pop();
                iterStack.pop();
                int pe = parentEdge[parentEdge.size - 1];
                parentEdge.pop();

                if (parent[v] != -1) {
                    // Обновляем low родителя.
                    if (low[v] < low[parent[v]])
                        low[parent[v]] = low[v];

                    // Забираем блок, если v — «корень» блока.
                    if (low[v] >= tin[parent[v]]) {
                        IntArray& vs = comps[nComp];
                        IntArray& es = compEdges[nComp];
                        vs.reserve(4);
                        es.reserve(8);

                        while (!edgeStack.empty()) {
                            int e = edgeStack.top();
                            edgeStack.pop();

                            // Ребро в ориентации входного файла.
                            int a = from[e];
                            int b = to[e];

                            // Добавляем вершины блока без дубликатов.
                            bool haveA = false, haveB = false;
                            for (int t = 0; t < vs.size; ++t) {
                                if (vs[t] == a) haveA = true;
                                if (vs[t] == b) haveB = true;
                            }
                            if (!haveA) vs.push(a);
                            if (!haveB) vs.push(b);

                            // Сохраняем ребро парой подряд.
                            es.push(a);
                            es.push(b);

                            if (e == pe) break;
                        }

                        compSizes[nComp] = vs.size;
                        compEdgeCount[nComp] = es.size / 2;
                        nComp++;
                    }
                }
            }
        }
    }

    delete[] tin;
    delete[] low;
    delete[] parent;
    delete[] off;
    delete[] adjTo;
    delete[] adjId;

    return nComp;
}

// ---------------------------------------------------------------
// Вывод результата в файл result.txt.
// Для КДС печатаем и вершины блока, и его рёбра (парами u-v).
// ---------------------------------------------------------------
void writeResult(const char* filename,
    int nSCC, IntArray* scc, int* sccSizes,
    int nBCC, IntArray* bcc, int* bccSizes,
    IntArray* bccEdges, int* bccEdgeCount)
{
    ofstream out(filename);
    if (!out) {
        cerr << "Cannot create output file: " << filename << "\n";
        return;
    }

    out << "strongly connected components of a directed graph: "
        << nSCC << " components\n";
    for (int i = 0; i < nSCC; ++i) {
        for (int j = 0; j < sccSizes[i]; ++j) {
            if (j) out << ' ';
            out << scc[i][j];
        }
        out << '\n';
    }

    out << '\n';

    out << "biconnected components of an undirected graph: "
        << nBCC << " components\n";
    for (int i = 0; i < nBCC; ++i) {
        out << "component " << (i + 1) << ":\n";

        out << "  vertices:";
        for (int j = 0; j < bccSizes[i]; ++j)
            out << ' ' << bcc[i][j];
        out << '\n';

        out << "  edges:";
        for (int j = 0; j < bccEdgeCount[i]; ++j) {
            int a = bccEdges[i][2 * j];
            int b = bccEdges[i][2 * j + 1];
            out << ' ' << a << '-' << b;
        }
        out << '\n';
    }

    out.close();
}

// ---------------------------------------------------------------
// main
// ---------------------------------------------------------------
int main(int argc, char** argv)
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    int n = 0, m = 0;
    int* from = 0;
    int* to = 0;

    if (!readGraph(argv[1], n, m, from, to))
        return 1;

    // --- Компоненты сильной связности ---
    IntArray* scc = 0;
    int* sccSizes = 0;
    int nSCC = stronglyConnectedComponents(n, m, from, to,
        scc, sccSizes);

    // --- Компоненты двусвязности ---
    IntArray* bcc = 0;
    int* bccSizes = 0;
    IntArray* bccEdges = 0;
    int* bccEdgeCount = 0;
    int nBCC = biconnectedComponents(n, m, from, to,
        bcc, bccSizes, bccEdges, bccEdgeCount);

    // --- Запись результата ---
    writeResult("result.txt",
        nSCC, scc, sccSizes,
        nBCC, bcc, bccSizes, bccEdges, bccEdgeCount);

    // --- Освобождение памяти ---
    delete[] from;
    delete[] to;

    if (scc) {
        delete[] scc;
        delete[] sccSizes;
    }
    if (bcc) {
        delete[] bcc;
        delete[] bccSizes;
        delete[] bccEdges;
        delete[] bccEdgeCount;
    }

    cout << "Done.\n";
    system("result.txt");

    return 0;
}