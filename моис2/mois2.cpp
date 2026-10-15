#include <iostream>
#include <fstream>

using namespace std;

struct node {
	int v = 0; //значение текущей вершины
	int* con = nullptr; //массив соседних вершин
	int neighbors = 0; //количество соседей
	bool passed = false; //"пройдено"
};

class graph {
private:
	int n = 0; //количество вершин
	node* nodes; //массив вершин (то есть сам граф)
	int components = 0; //количество компонент связности
	bool dirac_theoreme = false; //достаточное условие существования гамильтонового цикла

public:
	graph(ifstream& f) { //конструктор, создающий граф
		f >> n;

		if (n <= 0) {
			cout << "Impossible graph. Correct the input file\n";
			exit(1);
		}

		nodes = new node[n]; //выделяем динамическую память под массив вершин

		for (int i = 0; i < n; i++) {
			f >> nodes[i].v; //записываем значение вершины
			f >> nodes[i].neighbors; //записываем количество соседей

			nodes[i].con = new int[nodes[i].neighbors]; //выделяем динамическую память под массив смежных вершин
			for (int j = 0; j < nodes[i].neighbors; j++) {
				f >> nodes[i].con[j]; //заполняем массив смежных вершин
			}
		}
	}

	~graph() { //деструктор освобождения динамической памяти
		for (int i = 0; i < n; i++) {
			delete[] nodes[i].con;
		}
		delete[] nodes;
	}

	//подсчёт количества компонент связности поиском в глубину (итеративный DFS)
	void count_components() {
		int component = 0;
		for (int i = 0; i < n; i++) {
			if (!nodes[i].passed) {
				++component;

				int* stack = new int[n];
				int top = 0;
				nodes[i].passed = true;
				stack[top++] = i;

				while (top > 0) {
					int cur = stack[--top];
					for (int j = 0; j < nodes[cur].neighbors; j++) {
						int neighbor_idx = nodes[cur].con[j] - 1;
						if (!nodes[neighbor_idx].passed) {
							nodes[neighbor_idx].passed = true;
							stack[top++] = neighbor_idx;
						}
					}
				}
				delete[] stack;
			}
		}
		this->components = component;
	}

	//проверка необходимого и достаточного условия существования эйлерового цикла
	bool does_it_have_eulerian_cycle() {
		if (n <= 1) {
			cout << "This graph can't have eulerian cycle\n";
			return false;
		}

		for (int i = 0; i < n; i++) {
			if (nodes[i].neighbors % 2 != 0) {
				cout << "This graph can't have eulerian cycle\n";
				return false;
			}
		}

		if (components != 1) {
			cout << "This graph can't have eulerian cycle\n";
			return false;
		}

		return true;
	}

	//построение эйлерова цикла (алгоритм Хирохито, итеративный)
	void build_eulerian_cycle(ofstream& f) {
		// 1. Считаем общее число рёбер (каждое учтено дважды)
		int total_edges = 0;
		for (int i = 0; i < n; i++) total_edges += nodes[i].neighbors;

		// 2. Отображение "значение вершины -> индекс в массиве nodes"
		int max_val = 0;
		for (int i = 0; i < n; i++)
			if (nodes[i].v > max_val) max_val = nodes[i].v;

		int* val_to_idx = new int[max_val + 1];
		for (int i = 0; i <= max_val; i++) val_to_idx[i] = -1;
		for (int i = 0; i < n; i++) val_to_idx[nodes[i].v] = i;

		// 3. ptr[i] — указатель на следующее неиспользованное ребро у вершины i
		int* ptr = new int[n];
		for (int i = 0; i < n; i++) ptr[i] = 0;

		// массив для пометки использованных рёбер
		bool** edge_used = new bool* [n];
		for (int i = 0; i < n; i++) {
			edge_used[i] = new bool[nodes[i].neighbors];
			for (int j = 0; j < nodes[i].neighbors; j++)
				edge_used[i][j] = false;
		}

		// 4. Стек и итоговый цикл
		int* stack = new int[total_edges + 1];
		int* cycle = new int[total_edges + 1];
		int stack_top = 0;
		int cycle_len = 0;

		stack[stack_top++] = 0; // стартуем с вершины с индексом 0

		while (stack_top > 0) {
			int cur = stack[stack_top - 1];

			// ищем следующее НЕиспользованное ребро
			int next_edge = -1;
			while (ptr[cur] < nodes[cur].neighbors) {
				if (!edge_used[cur][ptr[cur]]) {
					next_edge = ptr[cur];
					break;
				}
				ptr[cur]++;
			}

			if (next_edge != -1) {
				int neighbor_val = nodes[cur].con[next_edge];
				int neighbor_idx = val_to_idx[neighbor_val];

				// помечаем ребро использованным с обеих сторон
				edge_used[cur][next_edge] = true;
				for (int j = 0; j < nodes[neighbor_idx].neighbors; j++) {
					if (nodes[neighbor_idx].con[j] == nodes[cur].v &&
						!edge_used[neighbor_idx][j]) {
						edge_used[neighbor_idx][j] = true;
						break;
					}
				}

				stack[stack_top++] = neighbor_idx;
			}
			else {
				// все рёбра использованы — вершина идёт в цикл
				cycle[cycle_len++] = cur;
				stack_top--;
			}
		}

		// 5. Запись в файл в формате x1 x2 ... x1
		//    Цикл строится с конца, поэтому выводим в обратном порядке,
		//    затем замыкаем стартовой вершиной.
		f << "eulerian cycle: ";
		for (int i = cycle_len - 1; i >= 0; i--) {
			f << nodes[cycle[i]].v << " ";
		}
		// Замыкающая вершина — та, с которой начали (cycle[cycle_len-1] — это старт)
		f << nodes[cycle[cycle_len - 1]].v;

		for (int i = 0; i < n; i++) delete[] edge_used[i];
		delete[] edge_used;

		delete[] val_to_idx;
		delete[] ptr;
		delete[] stack;
		delete[] cycle;

		cout << "Eulerian cycle has been built successfully\n";
	}

	int count_unvisited_neighbors(int u, const bool* used) {
		int cnt = 0;
		for (int j = 0; j < nodes[u].neighbors; j++) {
			int w = nodes[u].con[j] - 1;
			if (!used[w]) cnt++;
		}
		return cnt;
	}

	void build_hamiltonian_cycle(ofstream& f) {
		// --- общие проверки для любой ветки ---
		if (n < 3) {
			cout << "This graph can't have hamiltonian cycle\n";
			return;
		}

		if (components != 1) {
			cout << "This graph can't have hamiltonian cycle\n";
			return;
		}

		// --- проверка теоремы Дирака: deg(v) >= n/2 ---
		// для нечётного n используем 2*deg >= n
		dirac_theoreme = true;
		for (int i = 0; i < n; i++) {
			if (2 * nodes[i].neighbors < n) {
				dirac_theoreme = false;
				break;
			}
		}

		// --- если теорема не выполнена, допроверяем необходимое условие deg(v) >= 2 ---
		if (!dirac_theoreme) {
			for (int i = 0; i < n; i++) {
				if (nodes[i].neighbors < 2) {
					cout << "This graph can't have hamiltonian cycle\n";
					return;
				}
			}
		}

		bool found = false;
		int* path = new int[n + 1];
		int path_len = 0;

		if (dirac_theoreme) {
			// --- ветка для выполненной теоремы Дирака (с сортировкой Варнсдорфа) ---
			bool* used = new bool[n];
			for (int i = 0; i < n; i++) used[i] = false;

			struct Frame {
				int v;
				int next;
				int* order;
				int order_n;
			};
			Frame* st = new Frame[n + 1];
			int st_size = 0;

			used[0] = true;
			path[path_len++] = 0;

			Frame start;
			start.v = 0;
			start.next = 0;
			start.order_n = nodes[0].neighbors;
			start.order = new int[start.order_n];
			for (int j = 0; j < start.order_n; j++)
				start.order[j] = nodes[0].con[j] - 1;

			// Сортировка Варнсдорфа
			for (int a = 0; a < start.order_n - 1; a++) {
				for (int b = 0; b < start.order_n - 1 - a; b++) {
					int u1 = start.order[b];
					int u2 = start.order[b + 1];
					if (count_unvisited_neighbors(u1, used) >
						count_unvisited_neighbors(u2, used)) {
						int t = start.order[b];
						start.order[b] = start.order[b + 1];
						start.order[b + 1] = t;
					}
				}
			}
			st[st_size++] = start;

			while (st_size > 0 && !found) {
				Frame& top = st[st_size - 1];

				if (path_len == n) {
					bool edge_to_start = false;
					for (int j = 0; j < nodes[top.v].neighbors; j++) {
						if (nodes[top.v].con[j] - 1 == 0) { edge_to_start = true; break; }
					}
					if (edge_to_start) {
						found = true;
						break;
					}
					used[top.v] = false;
					path_len--;
					delete[] top.order;
					st_size--;
					continue;
				}

				int chosen = -1;
				while (top.next < top.order_n) {
					int u = top.order[top.next++];
					if (!used[u]) { chosen = u; break; }
				}

				if (chosen == -1) {
					used[top.v] = false;
					path_len--;
					delete[] top.order;
					st_size--;
					continue;
				}

				used[chosen] = true;
				path[path_len++] = chosen;

				Frame nf;
				nf.v = chosen;
				nf.next = 0;
				nf.order_n = nodes[chosen].neighbors;
				nf.order = new int[nf.order_n];
				for (int j = 0; j < nf.order_n; j++)
					nf.order[j] = nodes[chosen].con[j] - 1;

				for (int a = 0; a < nf.order_n - 1; a++) {
					for (int b = 0; b < nf.order_n - 1 - a; b++) {
						int u1 = nf.order[b];
						int u2 = nf.order[b + 1];
						if (count_unvisited_neighbors(u1, used) >
							count_unvisited_neighbors(u2, used)) {
							int t = nf.order[b];
							nf.order[b] = nf.order[b + 1];
							nf.order[b + 1] = t;
						}
					}
				}
				st[st_size++] = nf;
			}

			for (int i = 0; i < st_size; i++) delete[] st[i].order;
			delete[] st;
			delete[] used;
		}
		else {
			// --- ветка для невыполненной теоремы Дирака (простой DFS с backtracking) ---
			bool* used = new bool[n];
			for (int i = 0; i < n; i++) used[i] = false;

			struct Frame {
				int v;
				int next;
			};
			Frame* st = new Frame[n + 1];
			int st_size = 0;

			used[0] = true;
			path[path_len++] = 0;
			st[st_size].v = 0;
			st[st_size].next = 0;
			st_size++;

			while (st_size > 0 && !found) {
				Frame& top = st[st_size - 1];

				if (path_len == n) {
					bool edge_to_start = false;
					for (int j = 0; j < nodes[top.v].neighbors; j++) {
						if (nodes[top.v].con[j] - 1 == 0) { edge_to_start = true; break; }
					}
					if (edge_to_start) {
						found = true;
						break;
					}
					used[top.v] = false;
					path_len--;
					st_size--;
					continue;
				}

				int chosen = -1;
				while (top.next < nodes[top.v].neighbors) {
					int u = nodes[top.v].con[top.next++] - 1;
					if (!used[u]) { chosen = u; break; }
				}

				if (chosen == -1) {
					used[top.v] = false;
					path_len--;
					st_size--;
					continue;
				}

				used[chosen] = true;
				path[path_len++] = chosen;
				st[st_size].v = chosen;
				st[st_size].next = 0;
				st_size++;
			}

			delete[] st;
			delete[] used;
		}

		// --- вывод результата ---
		if (found) {
			f << "hamiltonian cycle: ";
			for (int i = 0; i < path_len; i++)
				f << nodes[path[i]].v << " ";
			f << nodes[0].v;
			cout << "Hamiltonian cycle has been built successfully\n";
		}
		else {
			cout << "This graph can't have hamiltonian cycle\n";
		}

		delete[] path;
	}
};



int main(int argc, char* argv[]) {
	if (argc != 2) {
		cerr << "Wrong input. Use this format: " << argv[0] << " <file_name>\n";
		return 1;
	}
	system("color F0");

	ifstream in(argv[1]); //открываем файл для чтения
	if (!in) {
		cout << "Can't open input file\n";
		return 1;
	}

	graph g(in); //создаём граф

	in.close();
	ofstream out("result.txt");
	if (!out) {
		cout << "Can't open output file\n";
		return 1;
	}

	g.count_components(); //считаем количество компонент связности

	// --- эйлеров цикл ---
	if (g.does_it_have_eulerian_cycle()) {
		g.build_eulerian_cycle(out);
	}
	out << "\n";

	// --- гамильтонов цикл ---
	g.build_hamiltonian_cycle(out);
	out << "\n";

	out.close();
	system("result.txt");

	return 0;
}