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
// Построение списков смежности.
// adjStart[v] — начало списка соседей вершины v в массиве adjList.
// Поскольку мы работаем без векторов, используем CSR-формат:
//   cnt[v]      — число исходящих дуг из v,
//   offset[v]   — смещение начала в adjList,
//   adjList[k]  — k-й сосед.
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
    // временные позиции заполнения
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
// Итеративный поиск в глубину (DFS) на орграфе.
// order — порядок выхода вершин из DFS (post-order).
// Возвращает число компонент сильной связности.
//
// Используем два прохода алгоритма Косараю:
//   1) DFS по исходному графу, запоминаем порядок выхода;
//   2) DFS по обратному графу в порядке, обратном порядку выхода.
// ---------------------------------------------------------------
int stronglyConnectedComponents(int n, int m,
    const int* from, const int* to,
    IntArray*& comps, int*& compSizes)
{
    // --- Построим прямой и обратный списки смежности ---
    int* offF, * adjF;
    buildAdjacency(n, m, from, to, offF, adjF);

    int* offR, * adjR;
    buildAdjacency(n, m, to, from, offR, adjR);

    // --- Проход 1: итеративный DFS, порядок выхода ---
    bool* visited = new bool[n];
    for (int i = 0; i < n; ++i) visited[i] = false;

    IntArray order;            // порядок выхода (post-order)
    order.reserve(n);

    IntArray stack;            // стек вершин
    IntArray iter;             // стек текущих индексов в списке смежности
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
                // все соседи обработаны — вершина выходит из DFS
                order.push(v);
                stack.pop();
                iter.pop();
            }
        }
    }

    delete[] visited;

    // --- Проход 2: DFS по обратному графу в обратном порядке ---
    bool* used = new bool[n];
    for (int i = 0; i < n; ++i) used[i] = false;

    comps = new IntArray[n];   // компоненты
    compSizes = new int[n];
    int nComp = 0;

    for (int k = order.size - 1; k >= 0; --k) {
        int start = order[k];
        if (used[start]) continue;

        // новая компонента
        IntArray& c = comps[nComp];
        c.reserve(4);
        c.push(start);
        used[start] = true;

        // итеративный DFS по обратному графу
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
// Поиск компонент двусвязности (блоков) неориентированного графа.
// Используется алгоритм Тарьяна с итеративной реализацией DFS.
//
// Идея:
//   - DFS по неориентированному графу; для каждой вершины храним
//     глубину tin[v] и минимальную достижимую глубину low[v].
//   - Ребро (v, u) — мост, если low[u] > tin[v].
//   - Вершина v — точка сочленения, если у неё есть ребёнок u,
//     для которого low[u] >= tin[v] (и v — не корень DFS).
//   - Компонента двусвязности (блок) — множество рёбер,
//     накопленных в стеке между вызовами DFS.
// ---------------------------------------------------------------
int biconnectedComponents(int n, int m,
    const int* from, const int* to,
    IntArray*& comps, int*& compSizes,
    bool*& isArticulation, bool*& isBridge)
{
    // Построим неориентированный список смежности (CSR).
    // Каждое ребро входит дважды (в обе стороны).
    int m2 = 2 * m;
    int* eFrom = new int[m2 > 0 ? m2 : 1];
    int* eTo = new int[m2 > 0 ? m2 : 1];
    int* eId = new int[m2 > 0 ? m2 : 1];   // номер исходного ребра
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

    // Массивы алгоритма Тарьяна
    int* tin = new int[n];
    int* low = new int[n];
    int* parent = new int[n];
    for (int i = 0; i < n; ++i) {
        tin[i] = -1;
        low[i] = -1;
        parent[i] = -1;
    }
    isArticulation = new bool[n];
    isBridge = new bool[m > 0 ? m : 1];
    for (int i = 0; i < n; ++i) isArticulation[i] = false;
    for (int i = 0; i < m; ++i) isBridge[i] = false;

    // Стек рёбер (по id) — для накопления блока
    IntArray edgeStack;
    edgeStack.reserve(16);

    comps = new IntArray[n];
    compSizes = new int[n];
    int nComp = 0;

    int timer = 0;

    // Итеративный DFS-обход
    IntArray vertexStack;   // вершины
    IntArray iterStack;     // текущие позиции в списке смежности
    IntArray parentEdge;    // ребро, по которому пришли в вершину
    vertexStack.reserve(n);
    iterStack.reserve(n);
    parentEdge.reserve(n);

    for (int start = 0; start < n; ++start) {
        if (tin[start] != -1) continue;

        // Начинаем новый DFS из start (корень дерева DFS)
        tin[start] = low[start] = timer++;
        vertexStack.push(start);
        iterStack.push(off[start]);
        parentEdge.push(-1);

        int rootChildren = 0;

        while (!vertexStack.empty()) {
            int v = vertexStack.top();
            int& it = iterStack[iterStack.size - 1];

            if (it < off[v + 1]) {
                int u = adjTo[it];
                int id = adjId[it];
                it++;

                // Пропускаем ребро, по которому пришли (в обратную сторону)
                if (parent[v] == u && parentEdge[parentEdge.size - 1] == id)
                    continue;
                if (parent[v] == u && parentEdge[parentEdge.size - 1] == -1)
                    continue;

                if (tin[u] == -1) {
                    // Ребро дерева
                    parent[u] = v;
                    tin[u] = low[u] = timer++;
                    if (v == start) rootChildren++;

                    vertexStack.push(u);
                    iterStack.push(off[u]);
                    parentEdge.push(id);
                }
                else {
                    // Обратное ребро (u — предок или уже посещён)
                    if (tin[u] < low[v]) low[v] = tin[u];
                    // Ребро попадает в стек рёбер блока
                    // (только если u ещё «в стеке вершин» —
                    //  для неориентированного графа это значит tin[u] < tin[v])
                    if (tin[u] < tin[v])
                        edgeStack.push(id);
                }
            }
            else {
                // Все соседи обработаны — выходим из v
                vertexStack.pop();
                iterStack.pop();
                int pe = parentEdge[parentEdge.size - 1];
                parentEdge.pop();

                if (parent[v] != -1) {
                    // Обновляем low родителя
                    if (low[v] < low[parent[v]])
                        low[parent[v]] = low[v];

                    // Проверяем, является ли ребро (parent[v], v) мостом
                    if (low[v] > tin[parent[v]])
                        isBridge[pe] = true;

                    // Проверяем, является ли parent[v] точкой сочленения
                    if (low[v] >= tin[parent[v]] &&
                        parent[parent[v]] != -1)
                        isArticulation[parent[v]] = true;

                    // Если v — корень DFS и у него >1 ребёнка
                    if (parent[v] == start && rootChildren > 1)
                        isArticulation[start] = true;

                    // Если low[v] >= tin[parent[v]] — забираем блок
                    if (low[v] >= tin[parent[v]]) {
                        IntArray& c = comps[nComp];
                        c.reserve(4);
                        while (!edgeStack.empty()) {
                            int e = edgeStack.top();
                            edgeStack.pop();
                            c.push(e);
                            if (e == pe) break;
                        }
                        compSizes[nComp] = c.size;
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
// Вывод результата в файл result.txt
// ---------------------------------------------------------------
void writeResult(const char* filename,
    int nSCC, IntArray* scc, int* sccSizes,
    int nBCC, IntArray* bcc, int* bccSizes,
    int m, bool* isBridge, int n,
    bool* isArticulation)
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
        for (int j = 0; j < bccSizes[i]; ++j) {
            if (j) out << ' ';
            out << bcc[i][j];
        }
        out << '\n';
    }

    // Дополнительно: точки сочленения и мосты
    out << '\n';
    out << "articulation points:";
    bool anyArt = false;
    for (int i = 0; i < n; ++i) {
        if (isArticulation[i]) {
            out << ' ' << i;
            anyArt = true;
        }
    }
    if (!anyArt) out << " (none)";
    out << '\n';

    out << "bridges (by edge index):";
    bool anyBr = false;
    for (int i = 0; i < m; ++i) {
        if (isBridge[i]) {
            out << ' ' << i;
            anyBr = true;
        }
    }
    if (!anyBr) out << " (none)";
    out << '\n';

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
    bool* isArt = 0;
    bool* isBri = 0;
    int nBCC = biconnectedComponents(n, m, from, to,
        bcc, bccSizes,
        isArt, isBri);

    // --- Запись результата ---
    writeResult("result.txt",
        nSCC, scc, sccSizes,
        nBCC, bcc, bccSizes,
        m, isBri, n, isArt);

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
        delete[] isArt;
        delete[] isBri;
    }

    cout << "Done.\n";
    system("result.txt");

    return 0;
}