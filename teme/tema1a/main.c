#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <math.h>

// Structurile deja definite
typedef struct QueueNode {
    char *file_name;
    int file_id;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *front;
    QueueNode *rear;
} Queue;

typedef struct MapperResultNode {
    char *word;
    int *file_ids;
    int file_count;
    int file_capacity;
    struct MapperResultNode *next;
} MapperResultNode;

typedef struct MapperResult {
    MapperResultNode *head;
} MapperResult;

typedef struct ThreadData {
    int id;
    Queue *file_queue;
    MapperResult **mapper_results;
    pthread_mutex_t *mutex_mapper;
    pthread_barrier_t *barrier;
    int num_reducers;
} ThreadData;

// Funcții pentru Queue
Queue *create_queue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    return queue;
}

void enqueue(Queue *queue, const char *file_name, int file_id) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    node->file_name = strdup(file_name);
    node->file_id = file_id;
    node->next = NULL;
    if (queue->rear) {
        queue->rear->next = node;
    } else {
        queue->front = node;
    }
    queue->rear = node;
}

int dequeue(Queue *queue, char **file_name, int *file_id) {
    if (!queue->front) return 0;
    QueueNode *node = queue->front;
    *file_name = node->file_name;
    *file_id = node->file_id;
    queue->front = node->next;
    if (!queue->front) queue->rear = NULL;
    free(node);
    return 1;
}

void free_queue(Queue *queue) {
    char *file_name;
    int file_id;
    while (dequeue(queue, &file_name, &file_id)) {
        free(file_name);
    }
    free(queue);
}

// Funcții pentru MapperResult
MapperResult *create_mapper_result() {
    MapperResult *result = (MapperResult *)calloc(1, sizeof(MapperResult));
    if (!result) {
        fprintf(stderr, "Error: Failed to allocate MapperResult.\n");
        exit(EXIT_FAILURE);
    }
    return result;
}

void add_word_to_mapper_result(MapperResult *result, const char *word, int file_id) {
    if (!result || !word) return;

    MapperResultNode *current = result->head;
    while (current) {
        if (strcmp(current->word, word) == 0) {
            for (int i = 0; i < current->file_count; i++) {
                if (current->file_ids[i] == file_id) return;
            }
            if (current->file_count == current->file_capacity) {
                current->file_capacity = current->file_capacity ? current->file_capacity * 2 : 10;
                current->file_ids = realloc(current->file_ids, current->file_capacity * sizeof(int));
                if (!current->file_ids) {
                    fprintf(stderr, "Error: Failed to reallocate memory for file_ids.\n");
                    exit(EXIT_FAILURE);
                }
            }
            current->file_ids[current->file_count++] = file_id;
            return;
        }
        current = current->next;
    }

    MapperResultNode *node = (MapperResultNode *)malloc(sizeof(MapperResultNode));
    if (!node) {
        fprintf(stderr, "Error: Failed to allocate MapperResultNode.\n");
        exit(EXIT_FAILURE);
    }
    node->word = strdup(word);
    if (!node->word) {
        fprintf(stderr, "Error: Failed to allocate word.\n");
        free(node);
        exit(EXIT_FAILURE);
    }
    node->file_capacity = 10;
    node->file_ids = (int *)malloc(node->file_capacity * sizeof(int));
    if (!node->file_ids) {
        fprintf(stderr, "Error: Failed to allocate file_ids.\n");
        free(node->word);
        free(node);
        exit(EXIT_FAILURE);
    }
    node->file_ids[0] = file_id;
    node->file_count = 1;
    node->next = result->head;
    result->head = node;
}

void free_mapper_result(MapperResult *result) {
    MapperResultNode *current = result->head;
    while (current) {
        MapperResultNode *temp = current;
        free(current->word);
        free(current->file_ids);
        current = current->next;
        free(temp);
    }
    free(result);
}

// NormalizeWord function
char *normalize_word(const char *word) {
    char *normalized = (char *)malloc(strlen(word) + 1);
    char *p = normalized;
    while (*word) {
        if (isalpha((unsigned char)*word)) {
            *p++ = tolower((unsigned char)*word);
        }
        word++;
    }
    *p = '\0';
    return normalized;
}

// ReducerRange function
void get_reducer_range(int reducer_id, int num_reducers, char *start_letter, char *end_letter) {
    int letters_per_reducer = (int)ceil(26.0 / num_reducers);
    *start_letter = 'a' + reducer_id * letters_per_reducer;
    *end_letter = 'a' + (reducer_id + 1) * letters_per_reducer - 1;
    if (*end_letter > 'z') *end_letter = 'z';
}

// Mapper Function
void *mapper(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char *file_name;
    int file_id;

    while (1) {
        pthread_mutex_lock(data->mutex_mapper);
        if (!dequeue(data->file_queue, &file_name, &file_id)) {
            pthread_mutex_unlock(data->mutex_mapper);
            break;
        }
        pthread_mutex_unlock(data->mutex_mapper);

        FILE *file = fopen(file_name, "r");
        if (!file) {
            fprintf(stderr, "Error opening file: %s\n", file_name);
            free(file_name);
            continue;
        }

        char word[256];
        MapperResult *local_results = create_mapper_result();

        while (fscanf(file, "%255s", word) == 1) {
            char *normalized = normalize_word(word);
            if (strlen(normalized) > 0) {
                add_word_to_mapper_result(local_results, normalized, file_id);
            }
            free(normalized);
        }
        fclose(file);
        free(file_name);

        pthread_mutex_lock(data->mutex_mapper);
        MapperResultNode *current = local_results->head;
        while (current) {
            add_word_to_mapper_result(data->mapper_results[data->id], current->word, current->file_ids[0]);
            current = current->next;
        }
        pthread_mutex_unlock(data->mutex_mapper);
        free_mapper_result(local_results);
    }
    pthread_barrier_wait(data->barrier);
    return NULL;
}

// Reducer Function
void *reducer(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    pthread_barrier_wait(data->barrier);

    char start_letter, end_letter;
    get_reducer_range(data->id, data->num_reducers, &start_letter, &end_letter);

    MapperResult *local_results = create_mapper_result();

    for (int i = 0; i < data->num_reducers; i++) {
        MapperResultNode *current = data->mapper_results[i]->head;
        while (current) {
            if (!current->word) {
                current = current->next;
                continue;
            }
            char initial = tolower((unsigned char)current->word[0]);
            if (initial >= start_letter && initial <= end_letter) {
                for (int j = 0; j < current->file_count; j++) {
                    add_word_to_mapper_result(local_results, current->word, current->file_ids[j]);
                }
            }
            current = current->next;
        }
    }

    for (char letter = start_letter; letter <= end_letter; ++letter) {
        char filename[10];
        snprintf(filename, sizeof(filename), "%c.txt", letter);
        FILE *outfile = fopen(filename, "w");
        if (!outfile) {
            fprintf(stderr, "Error opening file: %s\n", filename);
            continue;
        }

        MapperResultNode *current = local_results->head;
        while (current) {
            if (tolower((unsigned char)current->word[0]) == letter) {
                fprintf(outfile, "%s:[", current->word);
                for (int j = 0; j < current->file_count; j++) {
                    fprintf(outfile, "%d%s", current->file_ids[j],
                            (j == current->file_count - 1) ? "" : " ");
                }
                fprintf(outfile, "]\n");
            }
            current = current->next;
        }

        fclose(outfile);
    }

    free_mapper_result(local_results);
    return NULL;
}

// Main Function
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_mappers> <num_reducers> <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_mappers = atoi(argv[1]);
    int num_reducers = atoi(argv[2]);
    const char *input_file = argv[3];

    FILE *infile = fopen(input_file, "r");
    if (!infile) {
        fprintf(stderr, "Error opening input file: %s\n", input_file);
        return EXIT_FAILURE;
    }

    Queue *file_queue = create_queue();
    int num_files;
    fscanf(infile, "%d", &num_files);

    for (int i = 1; i <= num_files; i++) {
        char file_name[256];
        fscanf(infile, "%s", file_name);
        enqueue(file_queue, file_name, i);
    }
    fclose(infile);

    MapperResult *mapper_results[num_mappers];
    for (int i = 0; i < num_mappers; i++) {
        mapper_results[i] = create_mapper_result();
    }

    pthread_mutex_t mutex_mapper;
    pthread_barrier_t barrier;
    pthread_mutex_init(&mutex_mapper, NULL);
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);

    pthread_t mappers[num_mappers];
    pthread_t reducers[num_reducers];

    ThreadData thread_data[num_mappers + num_reducers];
    for (int i = 0; i < num_mappers; i++) {
        thread_data[i] = (ThreadData){i, file_queue, mapper_results, &mutex_mapper, &barrier, num_reducers};
        pthread_create(&mappers[i], NULL, mapper, &thread_data[i]);
    }

    for (int i = 0; i < num_reducers; i++) {
        thread_data[num_mappers + i] = (ThreadData){i, NULL, mapper_results, NULL, &barrier, num_reducers};
        pthread_create(&reducers[i], NULL, reducer, &thread_data[num_mappers + i]);
    }

    for (int i = 0; i < num_mappers; i++) {
        pthread_join(mappers[i], NULL);
    }

    for (int i = 0; i < num_reducers; i++) {
        pthread_join(reducers[i], NULL);
    }

    for (int i = 0; i < num_mappers; i++) {
        free_mapper_result(mapper_results[i]);
    }

    free_queue(file_queue);
    pthread_mutex_destroy(&mutex_mapper);
    pthread_barrier_destroy(&barrier);

    return EXIT_SUCCESS;
}
