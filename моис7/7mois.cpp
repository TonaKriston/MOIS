#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

using namespace std;

// Функция для чтения перестановки из файла.
// Входной файл содержит одну строку из цифр, каждая цифра - элемент перестановки.
// Например "324615" означает перестановку 1->3, 2->2, 3->4, 4->6, 5->1, 6->5.
vector<int> readPermutation(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Error: cannot open input file: " << filename << endl;
        exit(1);
    }
    string s;
    fin >> s;
    fin.close();

    vector<int> perm(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        // Проверка, что символ - цифра
        if (!isdigit((unsigned char)s[i])) {
            cerr << "Error: input file must contain digits only" << endl;
            exit(1);
        }
        perm[i] = s[i] - '0';
        // Элементы перестановки должны быть в диапазоне 1..n
        if (perm[i] < 1 || perm[i] > (int)s.size()) {
            cerr << "Error: invalid permutation value" << endl;
            exit(1);
        }
    }
    return perm;
}

// Функция 1: представление перестановки в виде произведения независимых циклов.
// Возвращает строку вида "(61)(352)(4)".
string permutationToCycles(const vector<int>& perm) {
    int n = (int)perm.size();
    vector<bool> visited(n, false);
    string result;

    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;

        // Строим цикл, начиная с элемента i (перестановка в 1-индексации)
        string cycle = "(";
        int cur = i;
        while (!visited[cur]) {
            visited[cur] = true;
            cycle += to_string(perm[cur]);
            cur = perm[cur] - 1; // переходим к следующему элементу (обратно в 0-индексацию)
        }
        cycle += ")";
        result += cycle;
    }
    return result;
}

// Вспомогательная функция: НОД двух чисел
long long gcd(long long a, long long b) {
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// Вспомогательная функция: НОК двух чисел
long long lcm(long long a, long long b) {
    return a / gcd(a, b) * b;
}

// Функция 2: определение степени перестановки.
// Степень перестановки - НОК длин всех её независимых циклов.
long long permutationOrder(const vector<int>& perm) {
    int n = (int)perm.size();
    vector<bool> visited(n, false);
    long long order = 1;

    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;
        int length = 0;
        int cur = i;
        while (!visited[cur]) {
            visited[cur] = true;
            ++length;
            cur = perm[cur] - 1;
        }
        order = lcm(order, length);
    }
    return order;
}

// Функция 3: определение чётности перестановки через число транспозиций.
// Раскладываем перестановку в произведение транспозиций: количество транспозиций = n - (число циклов).
// Возвращает пару: (чётность, число транспозиций).
pair<bool, int> parityByTranspositions(const vector<int>& perm) {
    int n = (int)perm.size();
    vector<bool> visited(n, false);
    int cycleCount = 0;

    for (int i = 0; i < n; ++i) {
        if (visited[i]) continue;
        ++cycleCount;
        int cur = i;
        while (!visited[cur]) {
            visited[cur] = true;
            cur = perm[cur] - 1;
        }
    }

    int transpositions = n - cycleCount;
    // Перестановка чётная, если число транспозиций чётное
    bool isEven = (transpositions % 2 == 0);
    return { isEven, transpositions };
}

// Функция 4: определение чётности перестановки через число инверсий.
// Число инверсий - количество пар (i, j), i < j, таких что perm[i] > perm[j].
// Возвращает пару: (чётность, число инверсий).
pair<bool, int> parityByInversions(const vector<int>& perm) {
    int n = (int)perm.size();
    int inversions = 0;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (perm[i] > perm[j]) {
                ++inversions;
            }
        }
    }

    bool isEven = (inversions % 2 == 0);
    return { isEven, inversions };
}

int main(int argc, char* argv[]) {
    // Проверяем, что передан путь к входному файлу
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // Чтение перестановки
    vector<int> perm = readPermutation(argv[1]);

    // Открываем выходной файл (создаёт/перезаписывает)
    ofstream fout("result.txt");
    if (!fout.is_open()) {
        cerr << "Error: cannot open result.txt for writing" << endl;
        return 1;
    }

    // 1) Разложение в произведение независимых циклов
    string cycles = permutationToCycles(perm);
    fout << cycles << endl;

    // 2) Степень перестановки
    long long order = permutationOrder(perm);
    fout << "permutation order: " << order << endl;

    // 3) Чётность через число транспозиций
    {
        auto [isEven, count ] = parityByTranspositions(perm);
        fout << "permutation is " << (isEven ? "even" : "odd")
            << ", number of transpositions: " << count << endl;
    }

    // 4) Чётность через число инверсий
    {
        auto [isEven, count] = parityByInversions(perm);
        fout << "permutation is " << (isEven ? "even" : "odd")
            << ", number of inversions: " << count << endl;
    }

    fout.close();

    // Открываем result.txt для просмотра
    system("result.txt");

    return 0;
}