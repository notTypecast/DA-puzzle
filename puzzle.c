#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#define N 6

/* Puzzle description
 * NxN matrix M, of 0s and 1s
 * Transition operator T(i, j): flip bit at M[i, j], M[i-1, j], M[i+1, j], M[i, j-1], M[i, j+1] wherever applicable
 * Initial and goal state can vary, but are all 0s and all 1s respectively in this case
 */

// define a node, representing one puzzle state
typedef struct Node {
    short matrix[N][N];
    struct Node* parent;
    int parent_transition[2];
    short (*final_state)[N][N];
    int depth;
} Node;

// ##QUEUE##
// define a queue element
typedef struct QueueElement {
    Node* node;
    struct QueueElement* prev;
} QueueElement;


// define a queue
typedef struct Queue {
    QueueElement* head;
    QueueElement* tail;
} Queue;

void queue_push(Queue* queue, Node* new_node) {
    QueueElement* new_element = (QueueElement*) malloc(sizeof(QueueElement));
    new_element->node = new_node;
    new_element->prev = NULL;

    if (queue->head == NULL) {
        queue->head = new_element;
        queue->tail = new_element;
        return;
    }

    queue->tail->prev = new_element;
    queue->tail = new_element;
}

Node* queue_pop(Queue* queue) {
    if (queue->head == NULL) {
        return NULL;
    }
    Node* popped = queue->head->node;
    QueueElement* tmp = queue->head;
    queue->head = queue->head->prev;
    free(tmp);
    return popped;
}

// ##BINARY MIN HEAP##

// define a heap element
// contains node, and value given to node by a heuristic function
typedef struct HeapElement {
    Node* node;
    int hval;
} HeapElement;

// define a heap
// h is the heuristic function to be used on its elements to find their hval
typedef struct Heap {
    HeapElement* arr;
    int curr_size;
    int max_size;
    int (*h)(Node* n);
} Heap;

// Initialize a new heap
Heap* init_heap(int (*h)(Node* n)) {
    Heap* heap = (Heap*) malloc(sizeof(Heap));
    heap->curr_size = 0;
    heap->max_size = 16;
    heap->arr = (HeapElement*) malloc(sizeof(HeapElement)*heap->max_size);
    heap->h = h;
}

Heap* delete_heap(Heap* heap) {
    free(heap->arr);
    free(heap);
}

int heap_parent(int index) {
    return (index-1) / 2;
}

int heap_right_child(int index) {
    return 2*index+2;
}

int heap_left_child(int index) {
    return 2*index+1;
}

// Heapify up method
void heapify_up(Heap* heap, int index) {
    if (!index) {
        return;
    }
    int parent_index = heap_parent(index);
    if (heap->arr[index].hval < heap->arr[parent_index].hval) {
        HeapElement tmp = heap->arr[index];
        heap->arr[index] = heap->arr[parent_index];
        heap->arr[parent_index] = tmp;
        heapify_up(heap, parent_index);
    }
}

// Heapify down method
void heapify_down(Heap* heap, int index) {
    int left_child = heap_left_child(index);
    int right_child = heap_right_child(index);

    if (right_child >= heap->curr_size) {
        if (left_child < heap->curr_size && heap->arr[index].hval > heap->arr[left_child].hval) {
            HeapElement tmp = heap->arr[index];
            heap->arr[index] = heap->arr[left_child];
            heap->arr[left_child] = tmp;
        }
        return;
    }

    if (heap->arr[index].hval < heap->arr[left_child].hval && heap->arr[index].hval < heap->arr[right_child].hval) {
        return;
    }

    int index_of_min;
    if (heap->arr[left_child].hval <= heap->arr[right_child].hval) {
        index_of_min = left_child;
    }
    else {
        index_of_min = right_child;
    }
    HeapElement tmp = heap->arr[index];
    heap->arr[index] = heap->arr[index_of_min];
    heap->arr[index_of_min] = tmp;

    heapify_down(heap, index_of_min);
}

// add a new element to the heap
// if there is not enough space, allocate more
void heap_add(Heap* heap, Node* node) {
    if (heap->curr_size == heap->max_size) {
        heap->max_size += 16;
        heap->arr = realloc(heap->arr, sizeof(HeapElement)*heap->max_size);
    }

    heap->arr[heap->curr_size].node = node;
    heap->arr[heap->curr_size].hval = heap->h(node);

    heapify_up(heap, (heap->curr_size)++);
}

// remove an element from the heap
// TODO: possibly deallocate space when a lot of elements have been removed
Node* heap_remove(Heap* heap, int index) {
    if (index >= heap->curr_size || index < 0) {
        return NULL;
    }

    HeapElement tmp = heap->arr[index];
    heap->arr[index] = heap->arr[--(heap->curr_size)];
    heap->arr[heap->curr_size] = tmp;

    heapify_down(heap, index);

    return heap->arr[heap->curr_size].node;
}

// gets bit at given position of bit array
int get_bit(const int bit_array[], unsigned long pos) {
    return ((bit_array[pos/(sizeof(int)*8)] & (1 << (pos % (sizeof(int)*8)))) != 0);
}

// sets bit at given bit array position to 1
void set_bit(int bit_array[], unsigned long pos) {
    bit_array[pos/(sizeof(int)*8)] |= (1 << (pos % (sizeof(int)*8)));
}

// create a new node, initializing it to passed values
Node* create_node(short matrix[N][N], Node* parent, const int* parent_transition, short (*final_state)[N][N], int depth) {
    // allocate memory for new node
    Node* node = (Node*) malloc(sizeof(Node));
    // copy matrix values
    memcpy(node->matrix, matrix, N*N*sizeof(short));
    // save other values
    node->parent = parent;
    if (parent_transition != NULL) {
        node->parent_transition[0] = parent_transition[0];
        node->parent_transition[1] = parent_transition[1];
    }
    node->final_state = final_state;
    node->depth = depth;

    return node;
}

// uses transition operator defined by coordinates, returning new resulting node
Node* transition(Node* node, int coordinates[2]) {
    if (coordinates[0] < 0 || coordinates[0] >= N || coordinates[1] < 0 || coordinates[1] >= N) {
        return NULL;
    }

    Node* new_node = create_node(node->matrix, node, coordinates, node->final_state, node->depth + 1);

    new_node->matrix[coordinates[0]][coordinates[1]] = new_node->matrix[coordinates[0]][coordinates[1]] ? 0 : 1;

    if (coordinates[0]-1 >= 0) {
        new_node->matrix[coordinates[0]-1][coordinates[1]] = new_node->matrix[coordinates[0]-1][coordinates[1]] ? 0 : 1;
    }
    if (coordinates[0]+1 < N) {
        new_node->matrix[coordinates[0]+1][coordinates[1]] = new_node->matrix[coordinates[0]+1][coordinates[1]] ? 0 : 1;
    }
    if (coordinates[1]-1 >= 0) {
        new_node->matrix[coordinates[0]][coordinates[1]-1] = new_node->matrix[coordinates[0]][coordinates[1]-1] ? 0 : 1;
    }
    if (coordinates[1]+1 < N) {
        new_node->matrix[coordinates[0]][coordinates[1]+1] = new_node->matrix[coordinates[0]][coordinates[1]+1] ? 0 : 1;
    }

    return new_node;
}

// compares node's current state with final state, returns true if state is final
bool is_final(Node* node) {
    return !memcmp(node->matrix, node->final_state, N*N*sizeof(short));
}

// "hashes" a given node
// hashes are created by considering the matrix values of a node as a single binary number, with left to right and
// top to bottom representing lowering significance
// i.e [1 0; 0 1] would be 1001, or 9
// in this way, each state has a unique hash value
unsigned long hash_node(Node* node) {
    unsigned long hash = 0;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            hash <<= 1;
            hash |= node->matrix[i][j];
        }
    }

    return hash;
}

// displays the matrix stored in passed node
void print_node(Node* node) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%d ", node->matrix[i][j]);
        }
        printf("\n");
    }
}

// displays path from root node to given node
void print_path_to_root(Node* node) {
    Node* curr = node;

    Node* next = curr->parent;
    curr->parent = NULL;

    // reverse path to root
    while (next != NULL) {
        Node* tmp = next->parent;
        next->parent = curr;
        curr = next;
        next = tmp;
    }

    // current now contains root

    printf("Initial state:\n");
    print_node(curr);
    curr = curr->parent;

    int steps = 0;

    while (curr != NULL) {
        printf("T: (%d, %d) ->\n", curr->parent_transition[0], curr->parent_transition[1]);
        print_node(curr);
        curr = curr->parent;
        ++steps;
    }

    printf("\nSolution has %d steps.\n", steps);
}

void dealloc_path_to_root(Node* node) {
    Node* curr = node;
    Node* next;

    while (curr != NULL) {
        next = curr->parent;
        free(curr);
        curr = next;
    }
}

/* BFS for puzzle
 * IMPLEMENTATION NOTES
 * Time complexity: O(N^2d)
 * Space complexity: O(N^2d)
 * where d: depth of the best solution for NxN matrix
 * This is because, for BFS, complexity is O(b^d); b is N^2 (N^2 transitions per state), whereas d will be the depth
 * of the solution closest to root
 * For this reason, it becomes nearly impossible to run this on an average computer for N>5
 * Average space required for each N:
 * N=1: 2 bits
 * N=2: 16 bits
 * N=3: 512 bits
 * N=4: 64 Kbits
 * N=5: 32 Mbits
 * N=6: 64 Gbits
 * N=7: 512 Tbits
 */
Node* BFS(Node* start_node) {
    if (is_final(start_node)) {
        return start_node;
    }

    // keep "visited" set as bit array
    // allocating a total of 2^(N^2)/8 elements, since each is of size 1 byte (or 8 bits), so each represents 8 states
    // the value of bit i indicates whether node with hash=i has been visited
    int* visited = (int*) calloc((unsigned long)ceil(pow(2, N*N)/8), 1);
    if (visited == NULL) {
        perror("Not enough memory");
        exit(1);
    }
    int visited_length = 0;
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue_push(queue, start_node);
    int queue_length = 1;

    while (queue->head != NULL) {
        Node* current = queue_pop(queue);
        --queue_length;

        printf("\rDepth: %d, visited set length: %d, queue length: %d", current->depth, visited_length, queue_length);

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int coordinates[2] = {i, j};
                Node* new_node = transition(current, coordinates);
                unsigned long node_hash = hash_node(new_node);

                if (get_bit(visited, node_hash)) {
                    free(new_node);
                    continue;
                }

                if (is_final(new_node)) {
                    free(queue);
                    free(visited);
                    printf("\n");
                    return new_node;
                }

                queue_push(queue, new_node);
                set_bit(visited, node_hash);
                ++visited_length;
                ++queue_length;
            }
        }

    }

    free(queue);
    free(visited);
    printf("\n");
    return NULL;
}

/* "Heuristic" for BestFS returning a random integer 0-39
 * Used to demonstrate the importance of the heuristic function
 * Running BestFS with this function as a heuristic will essentially cause it to make a random choice each step,
 * resulting in a very non-optimal solution being found with (usually) hundreds of steps
 */
int hrand(Node* n) {
    return rand() % 40;
}

/* Heuristic 1 for BestFS
 * Returns number of 0s in matrix of node
 */
int h1(Node* n) {
    int zeroes = 0;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!n->matrix[i][j]) {
                ++zeroes;
            }
        }
    }

    return zeroes;
}

/* Heuristic 2 for BestFS
 * Looks for patterns of 0s that match transitions and adds 1 for each one (no overlap)
 * Adds 1 for each 0 not in such a pattern
 */
int h2(Node* n) {
    int hval = 0;

    bool recognized[N][N] = {0};

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            // if cell is 1 or was previously included in pattern, skip
            if (n->matrix[i][j] || recognized[i][j]) {
                continue;
            }
            // determine which cells contain unrecognized 0s
            bool above = i-1 >= 0;
            bool below = i+1 < N;
            bool left = j-1 >= 0;
            bool right = j+1 < N;

            // look for cross
            if (above && below && left && right) {
                if (!(n->matrix[i-1][j] || recognized[i-1][j] || n->matrix[i+1][j] || recognized[i+1][j] ||
                      n->matrix[i][j-1] || recognized[i][j-1] || n->matrix[i][j+1] || recognized[i][j+1])) {
                    hval += 1;
                    recognized[i][j] = true;
                    recognized[i-1][j] = true;
                    recognized[i+1][j] = true;
                    recognized[i][j-1] = true;
                    recognized[i][j+1] = true;
                }
            }
                // look for top-right-bottom piece
            else if (above && right && below) {
                if (!(n->matrix[i-1][j] || recognized[i-1][j] || n->matrix[i+1][j] || recognized[i+1][j] ||
                      n->matrix[i][j+1] || recognized[i][j+1])) {
                    hval += 1;
                    recognized[i][j] = true;
                    recognized[i-1][j] = true;
                    recognized[i+1][j] = true;
                    recognized[i][j+1] = true;
                }
            }
                // look for top-left-bottom piece
            else if (above && left && below) {
                if (!(n->matrix[i-1][j] || recognized[i-1][j] || n->matrix[i+1][j] || recognized[i+1][j] ||
                      n->matrix[i][j-1] || recognized[i][j-1])) {
                    hval += 1;
                    recognized[i][j] = true;
                    recognized[i-1][j] = true;
                    recognized[i+1][j] = true;
                    recognized[i][j-1] = true;
                }
            }
                // look for right-bottom piece
            else if (right && below) {
                if (!(n->matrix[i+1][j] || recognized[i+1][j] || n->matrix[i][j+1] || recognized[i][j+1])) {
                    hval += 1;
                    recognized[i][j] = true;
                    recognized[i+1][j] = true;
                    recognized[i][j+1] = true;
                }
            }
                // look for top-left piece
            else if (above && left) {
                if (!(n->matrix[i-1][j] || recognized[i-1][j] || n->matrix[i][j-1] || recognized[i][j-1])) {
                    hval += 1;
                    recognized[i][j] = true;
                    recognized[i-1][j] = true;
                    recognized[i][j-1] = true;
                }
            }

        }
    }

    // for each unrecognized 0, add 1 to hval
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!(n->matrix[i][j] || recognized[i][j])) {
                hval += 1;
            }
        }
    }

    return hval;
}

/* Heuristic 3 for BestFS
 * Adds a for each 0 with an adjacent 0, b for each 0 surrounded by 1s
 * For different values of the ratio a/b, different solutions can be found
 * For different values of N, different ratios work best, for instance:
 * -For N=5, ratio 0.5 returns solution with 27 steps, ratio 1/6 retuns solution with 53 steps
 * -For N=6, ratio 0.5 returns solution with 100 steps, ratio 1/6 returns solution with 86 steps
 * TODO: figure out a way to find the ideal ratio a/b, for a given N
 */
int h3(Node* n) {
    int hval = 0;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!n->matrix[i][j]) {
                if (i-1 >= 0 && !n->matrix[i-1][j] || i+1 < N && !n->matrix[i+1][j] || j-1 >= 0 && !n->matrix[i][j-1] ||
                j+1 < N && !n->matrix[i][j+1]) {
                    hval += 1;
                }
                else {
                    hval += 6;
                }
            }
        }
    }

    return hval;
}

/* BestFS for puzzle
 * Arguments: initial node, heuristic function
 */
Node* BestFS(Node* start_node, int (*h)(Node* n)) {
    if (is_final(start_node)) {
        return start_node;
    }

    int* visited = (int*) calloc((unsigned long)ceil(pow(2, N*N)/8), 1);
    int visited_length = 0;
    // create heap utilizing passed heuristic
    Heap* heap = init_heap(h);
    heap_add(heap, start_node);
    int heap_length = 1;

    Node* current;

    while ((current = heap_remove(heap, 0)) != NULL) {
        --heap_length;
        printf("\rDepth: %d, visited set length: %d, heap length: %d", current->depth, visited_length, heap_length);

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int coordinates[2] = {i, j};
                Node* new_node = transition(current, coordinates);
                unsigned long node_hash = hash_node(new_node);

                if (get_bit(visited, node_hash)) {
                    free(new_node);
                    continue;
                }

                if (is_final(new_node)) {
                    delete_heap(heap);
                    printf("\n");
                    return new_node;
                }

                heap_add(heap, new_node);
                set_bit(visited, node_hash);
                ++visited_length;
                ++heap_length;
            }
        }
    }

    delete_heap(heap);
    printf("\n");
    return NULL;
}

int main(void) {
    srand(time(NULL));
    short initial_state[N][N];
    short final_state[N][N];

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            initial_state[i][j] = 0;
            final_state[i][j] = 1;
        }
    }

    Node* initial = create_node(initial_state, NULL, NULL, &final_state, 0);
    Node* result = BestFS(initial, h3);
    //Node* result = BFS(initial);

    if (result == NULL) {
        printf("Solution not found.\n");
    }
    else {
        print_path_to_root(result);
        dealloc_path_to_root(result);
    }

    return 0;
}