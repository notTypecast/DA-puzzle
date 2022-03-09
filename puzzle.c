#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#define N 5

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
int hash_node(Node* node) {
    int hash = 0;

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

/* BFS for puzzle
 * IMPLEMENTATION NOTES
 * Time complexity: O(N*2d)
 * Space complexity: O(N*2d)
 * where d: depth of the best solution for NxN matrix
 * This is because, for BFS, complexity is O(b^d); b is N^2 (N^2 transitions per state), whereas d will be the depth
 * of the solution closest to root
 * For this reason, it becomes nearly impossible to run this on an average computer for N>5
 * Average space required for each N:
 * N=1: 2 bytes
 * N=2: 16 bytes
 * N=3: 512 bytes
 * N=4: 64 KB
 * N=5: 32 MB
 * N=6: 64 GB
 * N=7: 512 TB
 */
Node* BFS(Node* start_node) {
    if (is_final(start_node)) {
        return start_node;
    }

    // keep "visited" set as array of booleans, of size 2^(N^2)
    // the value of visited[i] indicated whether node with hash=i has been visited
    bool* visited = (bool*) calloc((unsigned long)pow(2, N*N), sizeof(bool));
    int visited_length = 0;
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue_push(queue, start_node);

    while (queue->head != NULL) {
        Node* current = queue_pop(queue);

        printf("\rDepth: %d, visited set length: %d", current->depth, visited_length);

        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int coordinates[2] = {i, j};
                Node* new_node = transition(current, coordinates);
                int node_hash = hash_node(new_node);

                if (visited[node_hash]) {
                    free(new_node);
                    continue;
                }

                if (is_final(new_node)) {
                    printf("\n");
                    return new_node;
                }

                queue_push(queue, new_node);
                visited[node_hash] = true;
                ++visited_length;
            }
        }

    }

    printf("\n");
    return NULL;
}

int main(void) {
    short initial_state[N][N];
    short final_state[N][N];

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            initial_state[i][j] = 0;
            final_state[i][j] = 1;
        }
    }

    Node* initial = create_node(initial_state, NULL, NULL, &final_state, 0);

    Node* result = BFS(initial);

    if (result == NULL) {
        printf("Solution not found.\n");
    }
    else {
        print_path_to_root(result);
    }

    return 0;
}