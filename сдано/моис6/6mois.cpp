#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
using namespace std;

// ============================================================
//                    СТРУКТУРА УЗЛА ДЕРЕВА
// ============================================================
struct Node {
    char label;      // метка вершины
    Node* left;      // левый потомок
    Node* right;     // правый потомок
};

// -------- Создание нового узла --------
Node* createNode(char label) {
    Node* n = new Node;
    n->label = label;
    n->left = NULL;
    n->right = NULL;
    return n;
}

// -------- Поиск узла по метке (рекурсивный обход) --------
Node* findNode(Node* root, char label) {
    if (root == NULL) return NULL;
    if (root->label == label) return root;
    Node* res = findNode(root->left, label);
    if (res != NULL) return res;
    return findNode(root->right, label);
}

// -------- Освобождение памяти дерева --------
void freeTree(Node* root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// ============================================================
//                    ЧТЕНИЕ ДЕРЕВА ИЗ ФАЙЛА
// ============================================================
// Возвращает корень, либо NULL при ошибке открытия файла.
// Если файл открылся, но рёбер в нём нет — дерево считается ПУСТЫМ:
// корень = NULL, а признак "файл был пустым" сообщается через isEmptyFile.
Node* readTree(const char* filename, bool& isEmptyFile) {
    isEmptyFile = false;

    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "Cannot open file: " << filename << endl;
        return NULL; // ошибка открытия — isEmptyFile остаётся false
    }

    // Пробуем прочитать первое ребро. Если не удалось — файл пуст.
    char parent, child;
    if (!(fin >> parent >> child)) {
        isEmptyFile = true;
        fin.close();
        return NULL; // пустое дерево — корня нет
    }

    // Корень всегда 'a'
    Node* root = createNode('a');

    // Таблицы: "родитель -> сколько детей уже добавлено (0/1/2)"
    char parents[256];
    int counts[256];
    int parentCount = 0;

    // Первое ребро уже прочитано — обрабатываем его и остальные
    do {
        // Ищем родителя в таблице
        int idx = -1;
        for (int i = 0; i < parentCount; i++) {
            if (parents[i] == parent) { idx = i; break; }
        }
        if (idx == -1) {
            parents[parentCount] = parent;
            counts[parentCount] = 0;
            idx = parentCount;
            parentCount++;
        }

        // Находим узел-родителя в дереве
        Node* pnode = findNode(root, parent);
        if (pnode == NULL) {
            // родителя нет в дереве — данные некорректны, пропускаем
            continue;
        }

        // Добавляем потомка (если он не пустой '_')
        if (child != '_') {
            Node* cnode = createNode(child);
            if (counts[idx] == 0) {
                pnode->left = cnode;   // первый ребёнок — левый
            }
            else if (counts[idx] == 1) {
                pnode->right = cnode;  // второй ребёнок — правый
            }
        }
        counts[idx]++;
    } while (fin >> parent >> child);

    fin.close();
    return root;
}

// ============================================================
//              ПРЯМОЙ ОБХОД (preorder) СО СБОРОМ МЕТОК
// ============================================================
// Для пустого поддерева пишем '#', чтобы структура была однозначной.
// Пустое дерево целиком даст строку "#".
void preorder(Node* root, char* buf, int& pos) {
    if (root == NULL) {
        buf[pos++] = '#';
        return;
    }
    buf[pos++] = root->label;
    preorder(root->left, buf, pos);
    preorder(root->right, buf, pos);
}

// ============================================================
//         ФУНКЦИЯ 1: ПРОВЕРКА ИЗОМОРФНОСТИ ДЕРЕВЬЕВ
// ============================================================
// Два дерева изоморфны ⇔ совпадают их preorder-строки с маркерами '#'.
bool isIsomorphic(Node* t1, Node* t2) {
    char buf1[1024], buf2[1024];
    int p1 = 0, p2 = 0;
    preorder(t1, buf1, p1);
    preorder(t2, buf2, p2);
    buf1[p1] = '\0';
    buf2[p2] = '\0';
    return strcmp(buf1, buf2) == 0;
}

// ============================================================
//            ПЕЧАТЬ ДЕРЕВА В ФОРМАТЕ ВХОДНОГО ФАЙЛА
// ============================================================
void printTreeEdges(Node* root, ofstream& fout) {
    if (root == NULL) return;

    // Левый потомок
    if (root->left != NULL) {
        fout << root->label << " " << root->left->label << "\n";
    }
    else if (root->right != NULL) {
        // левого нет, но есть правый — нужен маркер '_'
        fout << root->label << " _\n";
    }

    // Правый потомок
    if (root->right != NULL) {
        fout << root->label << " " << root->right->label << "\n";
    }

    // Обходим поддеревья
    printTreeEdges(root->left, fout);
    printTreeEdges(root->right, fout);
}

// ============================================================
//       ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ПРИВЕДЕНИЯ К ИЗОМОРФНОСТИ
// ============================================================

// Считаем количество узлов
int countNodes(Node* root) {
    if (root == NULL) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

// Собираем метки дерева в массив (прямой обход)
void collectLabels(Node* root, char* arr, int& n) {
    if (root == NULL) return;
    arr[n++] = root->label;
    collectLabels(root->left, arr, n);
    collectLabels(root->right, arr, n);
}

// Перестраиваем дерево по структуре pattern, подставляя метки из labels
Node* rebuildLike(Node* pattern, char* labels, int& idx) {
    if (pattern == NULL) return NULL;
    Node* n = createNode(labels[idx++]);
    n->left = rebuildLike(pattern->left, labels, idx);
    n->right = rebuildLike(pattern->right, labels, idx);
    return n;
}

// ============================================================
//     ФУНКЦИЯ 2: ПРИВЕДЕНИЕ ДЕРЕВЬЕВ К ИЗОМОРФНОСТИ
// ============================================================
// Перестраиваем t2 по структуре t1. Если в t2 не хватает меток —
// добавляем новые ('z', 'y', ...). Если в t2 меток больше —
// лишние отбрасываются (эквивалент удаления вершин).
//
// Если эталон t1 пуст — результатом может быть только пустое дерево.
Node* makeIsomorphic(Node* t1, Node* t2) {
    if (t1 == NULL) {
        // «Изоморфно пустому» может быть только пустое дерево.
        return NULL;
    }

    int n1 = countNodes(t1);
    int n2 = countNodes(t2);

    // Собираем метки t2 (если t2 пуст — массив окажется пустым)
    char* labels = new char[n1 > 0 ? n1 : 1];
    int cnt = 0;
    collectLabels(t2, labels, cnt);

    // Если меток меньше, чем нужно — дополняем 'z','y','x',...
    if (n2 < n1) {
        char* tmp = new char[n1];
        for (int i = 0; i < n2; i++) tmp[i] = labels[i];
        for (int i = n2; i < n1; i++) {
            int k = i - n2;
            char c = (char)('z' - (k % 26)); // защита от выхода за 'a'
            tmp[i] = c;
        }
        delete[] labels;
        labels = tmp;
    }

    int idx = 0;
    Node* result = rebuildLike(t1, labels, idx);
    delete[] labels;
    return result;
}

// ============================================================
//              ФУНКЦИЯ 3: ВЫСОТА ДЕРЕВА
// ============================================================
// height(NULL) = 0 — пустое дерево имеет высоту 0.
int height(Node* root) {
    if (root == NULL) return 0;
    int hl = height(root->left);
    int hr = height(root->right);
    return 1 + (hl > hr ? hl : hr);
}

// ============================================================
//         ФУНКЦИЯ 4: ПРОВЕРКА СБАЛАНСИРОВАННОСТИ
// ============================================================
// Дерево сбалансировано, если для каждого узла
// |высота левого − высота правого| ≤ 1.
// Пустое дерево по соглашению считается сбалансированным.
bool isBalanced(Node* root) {
    if (root == NULL) return true;
    int hl = height(root->left);
    int hr = height(root->right);
    int diff = hl - hr;
    if (diff < 0) diff = -diff;
    if (diff > 1) return false;
    return isBalanced(root->left) && isBalanced(root->right);
}

// ============================================================
//                            MAIN
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: program file1 file2" << endl;
        return 1;
    }

    // Читаем оба дерева. Флаги empty1/empty2 отличают
    // «пустой файл» от «файл не открылся».
    bool empty1 = false, empty2 = false;
    Node* tree1 = readTree(argv[1], empty1);
    Node* tree2 = readTree(argv[2], empty2);

    // Если корень NULL, но файл не был пустым — была ошибка открытия
    if ((tree1 == NULL && !empty1) || (tree2 == NULL && !empty2)) {
        cout << "Error reading files" << endl;
        return 1;
    }

    // Открываем выходной файл
    ofstream fout("result.txt");
    if (!fout.is_open()) {
        cout << "Cannot create result.txt" << endl;
        return 1;
    }

    // Сообщим о пустых деревьях для наглядности
    if (empty1) fout << "First file is empty: tree 1 is empty (no nodes).\n";
    if (empty2) fout << "Second file is empty: tree 2 is empty (no nodes).\n";
    if (empty1 || empty2) fout << "\n";

    // --- Функция 3: высоты деревьев ---
    fout << "Height of the first tree: " << height(tree1) << "\n";
    fout << "Height of the second tree: " << height(tree2) << "\n\n";

    // --- Функция 4: сбалансированность ---
    fout << "First tree is ";
    if (isBalanced(tree1)) fout << "balanced";
    else                   fout << "not balanced";
    fout << ", second tree is ";
    if (isBalanced(tree2)) fout << "balanced";
    else                   fout << "not balanced";
    fout << "\n";

    // --- Функция 1: изоморфность ---
    bool iso = isIsomorphic(tree1, tree2);
    if (iso) fout << "Trees are isomorphic\n";
    else     fout << "Trees are not isomorphic\n";
    fout << "\n";

    // --- Функция 2: приведение к изоморфности ---
    if (!iso) {
        Node* newTree2 = makeIsomorphic(tree1, tree2);
        freeTree(tree2);
        tree2 = newTree2;

        fout << "After transformation, trees are now isomorphic.\n\n";

        fout << "Tree 1 (edge list):\n";
        if (tree1 == NULL) fout << "(empty tree)\n";
        else               printTreeEdges(tree1, fout);

        fout << "\nTree 2 (edge list):\n";
        if (tree2 == NULL) fout << "(empty tree)\n";
        else               printTreeEdges(tree2, fout);
        fout << "\n";
    }
    else {
        fout << "Transformation not required.\n\n";
    }

    fout.close();

    // Освобождение памяти
    freeTree(tree1);
    freeTree(tree2);

    // Открываем результат
    system("result.txt");

    return 0;
}