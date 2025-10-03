// Features: add, list (pending/completed), mark complete, delete, categories, save/load
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <ctime>

using namespace std;

struct Task {
    int id;
    string title;
    string category;
    bool done;
    string created_at;
};

vector<Task> tasks;
int next_id = 1;
const string DATA_FILE = "tasks.db";

// return current time as string
string now_string() {
    time_t t = time(nullptr);
    tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return string(buf);
}

// --- file handling ---
vector<string> split_csv_line(const string &line) {
    vector<string> out;
    string cur;
    for (char c : line) {
        if (c == ',') {
            out.push_back(cur);
            cur.clear();
        } else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

string escape_commas(const string &s) {
    string r;
    for (char c : s) {
        if (c == '\n' || c == '\r') r.push_back(' ');
        else if (c == ',') r.push_back(';');
        else r.push_back(c);
    }
    return r;
}

void load_tasks() {
    tasks.clear();
    ifstream in(DATA_FILE);
    if (!in.is_open()) return;
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        vector<string> parts = split_csv_line(line);
        if (parts.size() < 5) continue;
        Task t;
        try {
            t.id = stoi(parts[0]);
        } catch (...) { continue; }
        t.title = parts[1];
        t.category = parts[2];
        t.done = (parts[3] == "1");
        t.created_at = parts[4];
        tasks.push_back(t);
        if (t.id >= next_id) next_id = t.id + 1;
    }
    in.close();
}

void save_tasks() {
    ofstream out(DATA_FILE, ios::trunc);
   if (!out.is_open()) {
        cerr << "Error: could not write to " << DATA_FILE << "\n";
        return;
    }
    for (const Task &t : tasks) {
        out << t.id << "," << escape_commas(t.title) << "," << escape_commas(t.category)
            << "," << (t.done ? "1" : "0") << "," << t.created_at << "\n";
    }
    out.close();
}

void add_task() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter task title: ";
    string title;
    getline(cin, title);
    if (title.empty()) { cout << "Title cannot be empty.\n"; return; }
    cout << "Enter category (optional): ";
    string cat;
    getline(cin, cat);
    Task t;
    t.id = next_id++;
    t.title = title;
    t.category = cat;
    t.done = false;
    t.created_at = now_string();
    tasks.push_back(t);
    save_tasks();
    cout << "Task added (id=" << t.id << ").\n";
}

void print_task_row(const Task &t) {
    cout << "[" << (t.done ? "x" : " ") << "] "
         << "ID:" << t.id << " | " << t.title;
    if (!t.category.empty()) cout << " (" << t.category << ")";
    cout << "  -- created: " << t.created_at << "\n";
}

void list_tasks(bool show_done) {
    bool found = false;
    for (const Task &t : tasks) {
        if (t.done == show_done) {
            print_task_row(t);
            found = true;
        }
    }
    if (!found) cout << (show_done ? "No completed tasks.\n" : "No pending tasks.\n");
}

void list_all() {
    if (tasks.empty()) { cout << "No tasks yet.\n"; return; }
    vector<Task> sorted = tasks;
    sort(sorted.begin(), sorted.end(), [](const Task &a, const Task &b){
        if (a.done != b.done) return a.done < b.done;
        return a.id < b.id;
    });
    for (const Task &t : sorted) print_task_row(t);
}

void list_by_category() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter category to list (leave empty to list all categories): ";
    string cat;
    getline(cin, cat);
    bool found = false;
    for (const Task &t : tasks) {
        if (cat.empty() || t.category == cat) {
            print_task_row(t);
            found = true;
        }
    }
    if (!found) cout << "No tasks for that category.\n";
}

void mark_complete() {
    cout << "Enter task ID to toggle complete/incomplete: ";
    int id; if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Invalid input.\n"; return; }
    for (Task &t : tasks) {
        if (t.id == id) {
            t.done = !t.done;
            save_tasks();
            cout << "Task ID " << id << " marked " << (t.done ? "completed.\n" : "not completed.\n");
            return;
        }
    }
    cout << "Task ID not found.\n";
}

void delete_task() {
    cout << "Enter task ID to delete: ";
    int id; if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Invalid input.\n"; return; }
    auto it = remove_if(tasks.begin(), tasks.end(), [id](const Task &t){ return t.id == id; });
    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        save_tasks();
        cout << "Task ID " << id << " deleted.\n";
    } else {
        cout << "Task ID not found.\n";
    }
}

void show_help() {
    cout << R"(
Commands:
  1 - Add task
  2 - List pending tasks
  3 - List completed tasks
  4 - List all tasks
  5 - List tasks by category
  6 - Toggle complete/incomplete (by ID)
  7 - Delete task (by ID)
  8 - Save (explicit)
  9 - Load (explicit)
  h - Help
  q - Quit
)";
}

void menu_loop() {
    char cmd;
    while (true) {
        cout << "\nChoose command (h for help): ";
        if (!(cin >> cmd)) break;
        switch (cmd) {
            case '1': add_task(); break;
            case '2': list_tasks(false); break;
            case '3': list_tasks(true); break;
            case '4': list_all(); break;
            case '5': list_by_category(); break;
            case '6': mark_complete(); break;
            case '7': delete_task(); break;
            case '8': save_tasks(); cout << "Saved.\n"; break;
            case '9': load_tasks(); cout << "Loaded.\n"; break;
            case 'h': show_help(); break;
            case 'q': cout << "Goodbye!\n"; return;
            default: cout << "Unknown command. Press h for help.\n"; break;
        }
    }
}

int main() {
    cout << "=== Simple To-Do List (console) ===\n";
    load_tasks();
    show_help();
    menu_loop();
    return 0;
}
