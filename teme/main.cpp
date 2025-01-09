#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <pthread.h>
#include <algorithm>
#include <cctype>
#include <cmath>

struct ThreadData {
    int id;
    std::queue<std::pair<std::string, int>>* file_queue;
    std::vector<std::map<std::string, std::set<int>>>* mapper_results;
    pthread_mutex_t* mutex_mapper;
    pthread_barrier_t* barrier;
    int num_reducers;
};

std::string normalizeWord(const std::string& word) {
    std::string normalized;
    for (char c : word) {
        if (std::isalpha(c)) {
            normalized += std::tolower(c);
        }
    }
    return normalized;
}

std::pair<char, char> getReducerRange(int reducer_id, int num_reducers) {
    int letters_per_reducer = std::ceil(26.0 / num_reducers);
    char start_letter = 'a' + reducer_id * letters_per_reducer;
    char end_letter = std::min((char)('a' + (reducer_id + 1) * letters_per_reducer - 1), 'z');
    return {start_letter, end_letter};
}

void* mapper(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int mapper_id = data->id;

    while (true) {
        std::pair<std::string, int> file_info;

        pthread_mutex_lock(data->mutex_mapper);
        if (!data->file_queue->empty()) {
            file_info = data->file_queue->front();
            data->file_queue->pop();
        } else {
            pthread_mutex_unlock(data->mutex_mapper);
            break;
        }
        pthread_mutex_unlock(data->mutex_mapper);

        std::ifstream infile(file_info.first);
        if (!infile.is_open()) {
            std::cerr << "Eroare la deschiderea fișierului: " << file_info.first << "\n";
            continue;
        }

        std::string word;
        std::map<std::string, std::set<int>> local_results;

        while (infile >> word) {
            std::string normalized = normalizeWord(word);
            if (!normalized.empty()) {
                local_results[normalized].insert(file_info.second);
            }
        }

        infile.close();

        pthread_mutex_lock(data->mutex_mapper);
        for (const auto& [word, file_ids] : local_results) {
            (*data->mapper_results)[mapper_id][word].insert(file_ids.begin(), file_ids.end());
        }
        pthread_mutex_unlock(data->mutex_mapper);
    }

    pthread_barrier_wait(data->barrier);
    return nullptr;
}

void* reducer(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int reducer_id = data->id;

    pthread_barrier_wait(data->barrier);

    auto [start_letter, end_letter] = getReducerRange(reducer_id, data->num_reducers);
    std::map<std::string, std::set<int>> local_results;

    // Procesăm toate rezultatele mapperilor
    for (const auto& mapper_result : *data->mapper_results) {
        for (const auto& [word, file_ids] : mapper_result) {
            char initial = std::tolower(word[0]);
            if (initial >= start_letter && initial <= end_letter) {
                local_results[word].insert(file_ids.begin(), file_ids.end());
            }
        }
    }

    // Scrierea în fișier
    for (char letter = start_letter; letter <= end_letter; ++letter) {
        std::ofstream outfile(std::string(1, letter) + ".txt");
        if (outfile.is_open()) {
            // Colectăm cuvintele care încep cu litera curentă
            std::vector<std::pair<std::string, std::set<int>>> words_with_indices;
            for (const auto& [word, file_ids] : local_results) {
                if (word[0] == letter) {
                    words_with_indices.emplace_back(word, file_ids);
                }
            }

            // Sortăm: întâi după dimensiunea setului (descrescător), apoi alfabetic
            std::sort(words_with_indices.begin(), words_with_indices.end(),
                      [](const auto& a, const auto& b) {
                          if (a.second.size() != b.second.size()) {
                              return a.second.size() > b.second.size(); // Dimensiune descrescătoare
                          }
                          return a.first < b.first; // Alfabetic în caz de egalitate
                      });

            // Scriem în fișier
            for (const auto& [word, file_ids] : words_with_indices) {
                outfile << word << ":[";

                for (auto it = file_ids.begin(); it != file_ids.end(); ++it) {
                    outfile << *it;
                    if (std::next(it) != file_ids.end()) {
                        outfile << " ";
                    }
                }
                outfile << "]\n";
            }

            outfile.close();
        }
    }

    return nullptr;
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Utilizare: " << argv[0] << " <numar_mappers> <numar_reducers> <fisier_intrare>\n";
        return -1;
    }

    int num_mappers = std::stoi(argv[1]);
    int num_reducers = std::stoi(argv[2]);
    std::string input_file = argv[3];

    std::ifstream infile(input_file);

    std::queue<std::pair<std::string, int>> file_queue;
    int num_files;
    infile >> num_files;

    for (int i = 1; i <= num_files; ++i) {
        std::string file_name;
        infile >> file_name;
        file_queue.push({file_name, i});
    }
    infile.close();

    std::vector<std::map<std::string, std::set<int>>> mapper_results(num_mappers);

    pthread_mutex_t mutex_mapper;
    pthread_barrier_t barrier;

    pthread_mutex_init(&mutex_mapper, nullptr);
    pthread_barrier_init(&barrier, nullptr, num_mappers + num_reducers);

    pthread_t mappers[num_mappers];
    pthread_t reducers[num_reducers];

    ThreadData thread_data[num_mappers + num_reducers];
    for (int i = 0; i < num_mappers; ++i) {
        thread_data[i] = {i, &file_queue, &mapper_results, &mutex_mapper, &barrier, num_reducers};
        pthread_create(&mappers[i], nullptr, mapper, &thread_data[i]);
    }

    for (int i = 0; i < num_reducers; ++i) {
        thread_data[num_mappers + i] = {i, &file_queue, &mapper_results, &mutex_mapper, &barrier, num_reducers};
        pthread_create(&reducers[i], nullptr, reducer, &thread_data[num_mappers + i]);
    }

    for (int i = 0; i < num_mappers; ++i) {
        pthread_join(mappers[i], nullptr);
    }

    for (int i = 0; i < num_reducers; ++i) {
        pthread_join(reducers[i], nullptr);
    }

    pthread_mutex_destroy(&mutex_mapper);
    pthread_barrier_destroy(&barrier);

    return 0;
}
