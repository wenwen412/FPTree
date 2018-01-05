/*
 *  bpt.c
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.

 *  3. Neither the name of the copyright holder nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.

 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.

 *  Author:  Amittai Aviram
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "main.h"


// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
int order = DEFAULT_ORDER;


/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;


/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
node * queue = NULL;

/*The log_head is use to locate the array of a linked list of micro_log
 * micro_log is used when doing leaf splitting
 * each micro_log contains 2 pointers: p_current and p_new
 * See FPTree paper for more details*/
micro_log * log_array;

/*micro_log operations for single thread
 * Should change this in a multi-thread envorinment*/
micro_log * get_log(micro_log * log_array){
    return &(log_array[0]);
}
int reset_log(micro_log *log_array){
    log_array[0].p_current = NULL;
    log_array[0].p_new = NULL;
    return 0;
}


/**********************************************************************
 * ********   pmalloc()/ pfree()/ persistent() wrap psedo code  *******
 */

static inline void persistent(void *buf, uint32_t len, int fence)
{
    int a = 1;
}

void* pmalloc (size_t size){
    return malloc(size);
}

void pfree (void *ptr, size_t size){
    return free(ptr);
}

void *custom_calloc(size_t n, size_t size)
{
    return calloc(n, size);
    size_t total = n * size;
    void *p;
    p = pmalloc(total);

    if (!p) return NULL;

    return memset(p, 0, total);
}


/* A hash function that read key to generate hash fingerprint*/
unsigned char fingerprint(unsigned char key[], int key_len)
{
    unsigned hash = 0;
    for (int i=0; i<key_len; i++)
        hash = ((37 * hash) + key[i]) & 0xff;
    return hash;
}


static bool get  (byte,   byte);
static void set  (byte *, byte);
static void reset(byte *, byte);

/* CAREFUL WITH pos AND BITMAP SIZE! */

bool bitmapGet(byte *bitmap, int pos) {
/* gets the value of the bit at pos */
    if (pos > MAX_KEYS - 1)
        return BITMAP_NOTFOUND;
    return get(bitmap[pos/BIT], pos%BIT);
}

void bitmapSet(byte *bitmap, int pos) {
/* sets bit at pos to 1 */
    if (pos > MAX_KEYS - 1)
        return;
    set(&bitmap[pos/BIT], pos%BIT);
}

void bitmapReset(byte *bitmap, int pos) {
/* sets bit at pos to 0 */
    reset(&bitmap[pos/BIT], pos%BIT);
}

int bitmapSearch(byte *bitmap, bool n, int size, int start) {
/* Finds the first n value in bitmap after start */
/* size is the Bitmap size in bytes */
    int i;
    /* size is now the Bitmap size in bits */
    for(i = start+1, size *= BIT; i < size; i++)
        if(bitmapGet(bitmap,i) == n)
            return i;
    return BITMAP_NOTFOUND;
}

static bool get(byte a, byte pos) {
/* pos is something from 0 to 7*/
    return (a >> pos) & 1;
}

static void set(byte *a, byte pos) {
/* pos is something from 0 to 7*/
/* sets bit to 1 */
    *a |= 1 << pos;
}

static void reset(byte *a, byte pos) {
/* pos is something from 0 to 7*/
/* sets bit to 0 */
    *a &= ~(1 << pos);
}

// FUNCTION PROTOTYPES.






// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/* Copyright and license notice to user at startup.
 */
void license_notice( void ) {
    printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
                   "http://www.amittai.com\n", Version);
    printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
                   "type `show w'.\n"
                   "This is free software, and you are welcome to redistribute it\n"
                   "under certain conditions; type `show c' for details.\n\n");
}


/* Routine to print portion of GPL license to stdout.
 */
void print_license( int license_part ) {
    int start, end, line;
    FILE * fp;
    char buffer[0x100];

    switch(license_part) {
        case LICENSE_WARRANTEE:
            start = LICENSE_WARRANTEE_START;
            end = LICENSE_WARRANTEE_END;
            break;
        case LICENSE_CONDITIONS:
            start = LICENSE_CONDITIONS_START;
            end = LICENSE_CONDITIONS_END;
            break;
        default:
            return;
    }

    fp = fopen(LICENSE_FILE, "r");
    if (fp == NULL) {
        perror("print_license: fopen");
        exit(EXIT_FAILURE);
    }
    for (line = 0; line < start; line++)
        fgets(buffer, sizeof(buffer), fp);
    for ( ; line < end; line++) {
        fgets(buffer, sizeof(buffer), fp);
        printf("%s", buffer);
    }
    fclose(fp);
}


/* First message to the user.
 */
void usage_1( void ) {
    printf("B+ Tree of Order %d.\n", order);
    printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
                   "5th ed.\n\n"
                   "To build a B+ tree of a different order, start again and enter "
                   "the order\n"
                   "as an integer argument:  bpt <order>  ");
    printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
    printf("To start with input from a file of newline-delimited integers, \n"
                   "start again and enter the order followed by the filename:\n"
                   "bpt <order> <inputfile> .\n");
}


/* Second message to the user.
 */
void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
                   "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
                   "\tf <k>  -- Find the value under key <k>.\n"
                   "\tp <k> -- Print the path from the root to key k and its associated "
                   "value.\n"
                   "\tr <k1> <k2> -- Print the keys and values found in the range "
                   "[<k1>, <k2>\n"
                   "\td <k>  -- Delete key <k> and its associated value.\n"
                   "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
                   "same order.\n"
                   "\tt -- Print the B+ tree.\n"
                   "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
                   "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
                   "leaves.\n"
                   "\tq -- Quit. (Or use Ctl-D.)\n"
                   "\t? -- Print this help message.\n");
}


/* Brief usage note.
 */
void usage_3( void ) {
    printf("Usage: ./bpt [<order>]\n");
    printf("\twhere %d <= order <= %d .\n", MIN_ORDER, MAX_ORDER);
}


/* Helper function for printing the
 * tree out.  See print_tree.
 */
void enqueue( node * new_node ) {
    node * c;
    if (queue == NULL) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}


/* Helper function for printing the
 * tree out.  See print_tree.
 */
node * dequeue( void ) {
    node * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}


/* Prints the bottom row of keys
 * of the tree (with their respective
 * pointers, if the verbose_output flag is set.
 */
void print_leaves( node * root ) {
    int i;
    node * c = root;
    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    while (!IS_LEAF(c))
        c = c->pointers[0];
    while (true) {
        for (i = 0; i < c->num_keys; i++) {
            if (verbose_output)
                printf("%lx ", (unsigned long)c->pointers[i]);
            printf("%d ", c->keys[i]);
        }
        if (verbose_output)
            printf("%lx ", (unsigned long)c->pointers[order - 1]);
        if (c->pointers[order - 1] != NULL) {
            printf(" | ");
            c = c->pointers[order - 1];
        }
        else
            break;
    }
    printf("\n");
}


/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
//int height( node * root ) {
//    int h = 0;
//    node * c = root;
//    while (!c->is_leaf) {
//        c = c->pointers[0];
//        h++;
//    }
//    return h;
//}

int height( node * root ) {
    int h = 0;
    node * c = root;
    while (!IS_LEAF(c)){
        c = c->pointers[0];
        h++;
    }
    return h;
}


/* The following functions/ data structures are for sort function to get a split key*/
typedef struct key_index_pair{
    int key;
    int index;
}key_index_pair;

int comparator(const void *p, const void *q)
{
    int l = ((key_index_pair *)p)->key;
    int r = ((key_index_pair *)q)->key;
    return (l - r);
}
/* function to find a median point to split a leaf:
 * First store all valid key/index pairs in an array
 * Then sort on the array*/
int find_split_key(struct leaf_node *leaf)
{

    key_index_pair tmp_array[order - 1];
    int i, j =0;
    for (i = 0; i < order - 1 ; ++i) {
        if(bitmapGet(leaf->bitmap, i))
        {
            tmp_array[j].index = i;
            tmp_array[j++].key = leaf->keys[i];
        }

    }
    qsort (tmp_array, i, sizeof(key_index_pair),comparator);
    int index = (int)(i/2);
    return tmp_array[index].key;
}



/* Utility function to give the length in edges
 * of the path from any node to the root.
 * Wen Pan: remove this in the future as it's not necessary
 * FIXME: DO we still keep parent pointer in the nodes
 */

int path_to_root( node * root, node * child ) {
    int length = 0;
    node * c = child;
    while (c != root) {
        if(IS_LEAF(c))
            c = (LEAF_RAW(c))->parent;
        else
            c = c->parent;
        length++;
    }
    return length;
}


/* Prints the B+ tree in the command
 * line in level (rank) order, with the
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree( node * root ) {

    node * n = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root);
    while( queue != NULL ) {
        n = dequeue();
        if(IS_LEAF(n))
        {
            leaf_node *leaf_n = LEAF_RAW(n);
            if (leaf_n->parent != NULL && n == leaf_n->parent->pointers[0]) {
                new_rank = path_to_root(root, n);
                if (new_rank != rank) {
                    rank = new_rank;
                    printf("\n");
                }
            }
            if (verbose_output)
                printf("(%lx)", (unsigned long)n);
            for (i = 0; i < order - 1; i++) {
                if (verbose_output && bitmapGet(leaf_n->bitmap, i))
                    printf("%lx ", (unsigned long)n->pointers[i]);
                if (bitmapGet(leaf_n->bitmap, i))
                    printf("%d ", leaf_n->keys[i]);
            }
            if (verbose_output) {
                printf("%lx ", (unsigned long)leaf_n->pointers[order - 1]);

            }
            printf("| ");
        }
        else
        {
            if (n->parent != NULL && n == n->parent->pointers[0]) {
                new_rank = path_to_root( root, n );
                if (new_rank != rank) {
                    rank = new_rank;
                    printf("\n");
                }
            }
            if (verbose_output)
                printf("(%lx)", (unsigned long)n);
            for (i = 0; i < n->num_keys; i++) {
                if (verbose_output)
                    printf("%lx ", (unsigned long)n->pointers[i]);
                printf("%d ", n->keys[i]);
            }
            if (!IS_LEAF(n))
                for (i = 0; i <= n->num_keys; i++)
                    enqueue(n->pointers[i]);
            if (verbose_output) {
                printf("%lx ", (unsigned long)n->pointers[n->num_keys]);
            }
            printf("| ");}
    }
    printf("\n");
}


/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(node * root, int key, bool verbose) {
    record * r = find(root, key, verbose, NULL);
    if (r == NULL)
        printf("Record not found under key %d.\n", key);
    else
        printf("Record at %lx -- key %d, value %d.\n",
               (unsigned long)r, key, r->value);
}


/* Finds and prints the keys, pointers, and values within a range
 * of keys between key_start and key_end, including both bounds.
 */
void find_and_print_range( node * root, int key_start, int key_end,
                           bool verbose ) {
    int i;
    int array_size = key_end - key_start + 1;
    int returned_keys[array_size];
    void * returned_pointers[array_size];
    int num_found = find_range( root, key_start, key_end, verbose,
                                returned_keys, returned_pointers );
    if (!num_found)
        printf("None found.\n");
    else {
        for (i = 0; i < num_found; i++)
            printf("Key: %d   Location: %lx  Value: %d\n",
                   returned_keys[i],
                   (unsigned long)returned_pointers[i],
                   ((record *)
                           returned_pointers[i])->value);
    }
}


/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
int find_range( node * root, int key_start, int key_end, bool verbose,
                int returned_keys[], void * returned_pointers[]) {
    int i, num_found;
    num_found = 0;
    node * n = find_leaf( root, key_start, verbose );
    if (n == NULL) return 0;
    for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++) ;
    if (i == n->num_keys) return 0;
    while (n != NULL) {
        for ( ; i < n->num_keys && n->keys[i] <= key_end; i++) {
            returned_keys[num_found] = n->keys[i];
            returned_pointers[num_found] = n->pointers[i];
            num_found++;
        }
        n = n->pointers[order - 1];
        i = 0;
    }
    return num_found;
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 * For FPTree: We don't need to change anything
 * Since inner nodes doesnot need to be changed
 */
node * find_leaf( node * root, int key, bool verbose ) {
    int i = 0;
    node * c = root;
    if (c == NULL) {
        if (verbose)
            printf("Empty tree.\n");
        return c;
    }
    // while (!c->is_leaf) {
    while (!IS_LEAF(c)) {
        if (verbose) {
            printf("[");
            for (i = 0; i < c->num_keys - 1; i++)
                printf("%d ", c->keys[i]);
            printf("%d] ", c->keys[i]);
        }
        i = 0;
        while (i < c->num_keys) {
            if (key >= c->keys[i]) i++;
            else break;
        }
        if (verbose)
            printf("%d ->\n", i);
        c = (node *)c->pointers[i];
    }
    if (verbose) {
        printf("Leaf [");
        for (i = 0; i < c->num_keys - 1; i++)
            printf("%d ", c->keys[i]);
        printf("%d] ->\n", c->keys[i]);
    }
    return c;
}


/* Finds and returns the record to which
 * a key refers.
 */
record * find( node * root, int key, bool verbose, record * r ) {
    int i = 0;
    leaf_node * l;
    node * c = find_leaf( root, key, verbose );
    if (c == NULL) return NULL;
    if (!IS_LEAF(c)) return NULL;
    l = LEAF_RAW(c);
    // FIXME: replace with a Fingerptint prob function
    for (i = 0; i < order - 1; i++)
        if (bitmapGet(l->bitmap, i))
        {
            if (l->keys[i] == key)
                if (r != NULL){
                    pfree(l->pointers[i], sizeof(record));
                    l->pointers[i] = r;
                    break;
                }
                else
                    break;
        }
    if (i == order - 1)
        return NULL;
    else
        return (record *)l->pointers[i];
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION

/* Creates a new record to hold the value
 * to which a key refers.
 */
record * make_record(int value) {
    record * new_record = (record *)pmalloc(sizeof(record));
    if (new_record == NULL) {
        perror("Record creation.");
        exit(EXIT_FAILURE);
    }
    else {
        new_record->value = value;
        persistent(new_record, sizeof(record), 2);
    }
    return new_record;
}


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
node * make_node( bool is_leaf ) {
    node * new_node;
    if (is_leaf)
        new_node = pmalloc(sizeof(node));
    else
        new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        perror("Node creation.");
        exit(EXIT_FAILURE);
    }
    new_node->keys = malloc( (order - 1) * sizeof(int) );
    if (new_node->keys == NULL) {
        perror("New node keys array.");
        exit(EXIT_FAILURE);
    }
    new_node->pointers = malloc( order * sizeof(void *) );
    if (new_node->pointers == NULL) {
        perror("New node pointers array.");
        exit(EXIT_FAILURE);
    }
    // new_node->is_leaf = is_leaf;
    // persistent(&(new_node->is_leaf), sizeof(bool), 1);
    new_node->num_keys = 0;
    new_node->parent = NULL;
    // new_node->next = NULL;
    return new_node;
}

/* Creates a new leaf
 * and then adapting it appropriately.
 */
void ** pointers;
int * keys;
struct node * parent;
node * make_leaf( void ) {
    // node * leaf = make_node(true);
    node * FPT_node;
    leaf_node *l;

    l = pmalloc(sizeof(leaf_node));
    if (l == NULL) {
        perror("Leaf creation.");
        exit(EXIT_FAILURE);
        }
    for (int i = 0; i < 7; i++)
    {
        l->bitmap[i] = 0;
    }
    l->next = NULL;
    for (int i = 0; i < order - 1; i++)
    {
        l->fingerprint[i] = 0;
    }
    l->keys = pmalloc( (order - 1) * sizeof(int));
    if (l->keys == NULL) {
        perror("New node keys array.");
        exit(EXIT_FAILURE);
    }
    l->pointers = pmalloc( order * sizeof(void *) );
    if (l->pointers == NULL) {
        perror("New node pointers array.");
        exit(EXIT_FAILURE);
    }

    FPT_node = (node*)SET_LEAF(l);
    return FPT_node;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the node to the left of the key to be inserted.
 */
/*FIXME: now the problem is: sometimes when leaf becomes inner node
 * we forget to reorder the inner node*/
int get_left_index(node * parent, node * left) {

    int left_index = 0;

    while (left_index <= order - 1 &&
           parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
node * insert_into_leaf( node * l, int key, record * pointer ) {

    leaf_node *leaf = LEAF_RAW(l);
    int i, insertion_point;

    insertion_point = 0;
    // FPTree change: need to check bitmap to find a free spot
//    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
//        insertion_point++;
    while(bitmapGet(leaf->bitmap, insertion_point))
        insertion_point++;
    // CHANGE: unsorted doesnot need to shift key-pointer pairs
//    for (i = leaf->num_keys; i > insertion_point; i--) {
//        leaf->keys[i] = leaf->keys[i - 1];
//        leaf->pointers[i] = leaf->pointers[i - 1];
//    }

    leaf->keys[insertion_point] = key;
    leaf->pointers[insertion_point] = pointer;
    leaf->num_keys++;
    persistent(&(leaf->keys[insertion_point]), sizeof(leaf->keys[insertion_point]), 2);
    persistent(&(leaf->pointers[insertion_point]), sizeof(void *), 2);
    bitmapSet(leaf->bitmap, insertion_point);
    persistent(leaf->bitmap, sizeof(char)*7, 2);
    return l;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.

 */
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer) {

    node * new_leaf;
    int new_key, i;

    new_leaf = make_leaf();
    leaf_node *n_leaf = LEAF_RAW(new_leaf);

    micro_log * ulog = get_log(log_array);
    ulog->p_current = n_leaf;
    persistent(&(ulog->p_current), sizeof(void *), 2);

    leaf_node *o_leaf = LEAF_RAW(leaf);

    /*FPTree: copy the whole content to the new leaf
     * Attention: do not use memcpy(n_leaf, o_leaf, sizeof(leaf_node)) to copy all the content of the leaf node
     * Since pointers and keys are stored in a secondary array*/
    memcpy(n_leaf->bitmap, o_leaf->bitmap, sizeof(char)*7);
    memcpy(n_leaf->fingerprint, o_leaf->fingerprint, sizeof(char)*49);
    memcpy(n_leaf->keys, o_leaf->keys, sizeof(int)*(order - 1));
    memcpy(n_leaf->pointers, o_leaf->pointers, sizeof(void *)*order);
    persistent(n_leaf, sizeof(leaf_node), 2);

    /* find a split key thay evenly split keys into 2 leafs
     * small keys goes to old leaf*/
    int split_key = find_split_key(n_leaf);
    int  new_leaf_num_keys = 0, old_leaf_num_keys = 0;
    for (i = 0;  i <= MAX_KEYS - 1; ++ i) {
        if (bitmapGet(o_leaf->bitmap, i))
        {
            if(o_leaf->keys[i] >= split_key) {
                bitmapReset(o_leaf->bitmap, i);
                new_leaf_num_keys++;
            }
            else
                old_leaf_num_keys++;
        }
        else
            printf("?");
    }
    persistent(o_leaf->bitmap, sizeof(char)*7, 2);
    o_leaf->num_keys = old_leaf_num_keys;
    n_leaf->num_keys = new_leaf_num_keys;
    // reverse bitmap of new leaf
    for (int k = 0; k < 7; ++k) {
        n_leaf->bitmap[k] = n_leaf->bitmap[k] & (~o_leaf->bitmap[k]);
    }
    persistent(n_leaf->bitmap, sizeof(char)*7, 2);
    // Insert into new or old leaf depending on the key and split key

    if(key >= split_key)
        insert_into_leaf(new_leaf, key, pointer);
    else
        insert_into_leaf(leaf, key, pointer);

    // linked-list operation
    //n_leaf->pointers[order - 1] = o_leaf->pointers[order - 1];
    o_leaf->pointers[order - 1] = new_leaf;
    persistent(&(o_leaf->pointers[order - 1]), sizeof(void *), 2);

    n_leaf->parent = o_leaf->parent;
    reset_log(log_array);

    new_key = split_key;

    return insert_into_parent(root, leaf, new_key, new_leaf);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
node * insert_into_node(node * root, node * n,
                        int left_index, int key, node * right) {
    int i;

    for (i = n->num_keys; i > left_index; i--) {
        n->pointers[i + 1] = n->pointers[i];
        n->keys[i] = n->keys[i - 1];
    }
    n->pointers[left_index + 1] = right;
    n->keys[left_index] = key;
    n->num_keys++;
    return root;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
node * insert_into_node_after_splitting(node * root, node * old_node, int left_index,
                                        int key, node * right) {

    int i, j, split, k_prime;
    node * new_node, * child;
    int * temp_keys;
    node ** temp_pointers;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places.
     * Then create a new node and copy half of the
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_pointers = malloc( (order + 1) * sizeof(node *) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = malloc( order * sizeof(int) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */
    split = cut(order);
    new_node = make_node(false);
    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++) {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < order; i++, j++) {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pointers[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
        child = new_node->pointers[i];
        if(IS_LEAF(child))
            LEAF_RAW(child)->parent = new_node;
        else
            child->parent = new_node;
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root, old_node, k_prime, new_node);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
node * insert_into_parent(node * root, node * left, int key, node * right) {

    int left_index;
    node * parent;
    // FPTree FIXME: we might have to remove parent fields of each node
    // A new way to find parent is necessary
    if(IS_LEAF(left))
        parent = LEAF_RAW(left)->parent;
    else
        parent = left->parent;

    /* Case: new root. */

    if (parent == NULL)
        return insert_into_new_root(left, key, right);

    /* Case: leaf or node. (Remainder of
     * function body.)
     */

    /* Find the parent's pointer to the left
     * node.
     */

    left_index = get_left_index(parent, left);


    /* Simple case: the new key fits into the node.
     */

    if (parent->num_keys < order - 1)
        return insert_into_node(root, parent, left_index, key, right);

    /* Harder case:  split a node in order
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root, parent, left_index, key, right);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
node * insert_into_new_root(node * left, int key, node * right) {

    node * root = make_node(false);
    root->keys[0] = key;
    root->pointers[0] = left;
    root->pointers[1] = right;
    root->num_keys++;
    root->parent = NULL;
    if (IS_LEAF(left)) {
        leaf_node * left_leaf = LEAF_RAW(left);
        leaf_node * right_leaf = LEAF_RAW(right);
        left_leaf->parent = root;
        right_leaf->parent = root;
    }
    else{
        left->parent = root;
        right->parent = root;
    }
    return root;
}



/* First insertion:
 * start a new tree.
 */
node * start_new_tree(int key, record * pointer) {

    node * root = make_leaf();
    leaf_node * root_leaf = LEAF_RAW(root);
    root_leaf->keys[0] = key;
    root_leaf->pointers[0] = pointer;
    persistent(&(root_leaf->keys[0]), sizeof(root_leaf->keys[0]), 2);
    persistent(&(root_leaf->pointers[0]), sizeof(void *), 2);
    bitmapSet(root_leaf->bitmap, 0);
    persistent(root_leaf->bitmap, sizeof(char), 2);
    for (int i = 1; i <= order - 1; ++i) {
        root_leaf->pointers[i] = NULL;
    }

    root_leaf->parent = NULL;
    root_leaf->num_keys++;

    /*Initilize a global array for micro log
     *Since we are testing in a single thread environment,
     * 1 micro log is enough
     * Change this and related code when spliting a leaf for
     * multi-thread envorinment*/
    log_array = pmalloc(sizeof(micro_log));
    if (log_array == NULL)
        return NULL;

    return root;
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
node * insert( node * root, int key, int value ) {

    record * pointer;
    node * leaf;


    /* Create a new record for the
     * value.
     */
    pointer = make_record(value);

    /* The current implementation ignores
     * duplicates.
     * FIXME: should update value
     */
    if (find(root, key, false, pointer) != NULL)
        return root;



    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
    if (root == NULL)
        return start_new_tree(key, pointer);


    /* Case: the tree already exists.
     * (Rest of function body.)
     */
    leaf = find_leaf(root, key, false);
    leaf_node * leaf_raw = LEAF_RAW(leaf);

    /* Case: leaf has room for key and pointer.
     */
    if (leaf_raw->num_keys < order - 1) {
        leaf = insert_into_leaf(leaf, key, pointer);
        return root;
    }


    /* Case:  leaf must be split.
     */
    return insert_into_leaf_after_splitting(root, leaf, key, pointer);
}




// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;
    node * parent;
    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.
     * If n is the leftmost child, this means
     * return -1.
     */
    if(IS_LEAF(n))
        parent = LEAF_RAW(n)->parent;
    else
        parent = n->parent;
    for (i = 0; i <= parent->num_keys; i++)
        if (parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}

/*FPTree FIXME: fix the case for LEAF nodes*/
node * remove_entry_from_node(node * n, int key, node * pointer) {

    int i, num_pointers;


    // Remove the key and shift other keys accordingly.
    if (IS_LEAF(n))
    {
        leaf_node * leaf = LEAF_RAW(n);
        i = 0;
        while (leaf->keys[i] != key)
            i++;
        bitmapReset(leaf->bitmap, i);
        // One key fewer.
        leaf->num_keys--;

        return n;
    }
    else    //inner nodes
    {
        while (n->keys[i] != key)
            i++;
        for (++i; i < n->num_keys; i++)
            n->keys[i - 1] = n->keys[i];

        // Remove the pointer and shift other pointers accordingly.
        // First determine number of pointers.

        num_pointers = n->num_keys + 1;
        i = 0;
        // FPTree leaf node doesn't need to shift

        while (n->pointers[i] != pointer)
            i++;
        for (++i; i < num_pointers; i++)
            n->pointers[i - 1] = n->pointers[i];

        // One key fewer.
        n->num_keys--;

        // Set the other pointers to NULL for tidiness.
        // A leaf uses the last pointer to point to the next leaf.
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;
        return n;
    }

}


/* This funcition is called when a key/pointer is removed from a root node */
node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root.
     */

    // If it has a child, promote
    // the first (only) child
    // as the new root.

    if (!IS_LEAF(root)) {
        new_root = root->pointers[0];
        new_root->parent = NULL;

        free(root->keys);
        free(root->pointers);
        free(root);
    }

        // If it is a leaf (has no children),
        // then the whole tree is empty.

    else
        new_root = NULL;
        leaf_node *leaf = LEAF_RAW(root);
        free(leaf->keys);
        free(leaf->pointers);
        free(leaf);

    return new_root;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!IS_LEAF(n)) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

        /* In a leaf, append the keys and pointers of
         * n to the neighbor.
         * Set the neighbor's last pointer to point to
         * what had been n's right neighbor.
         * Change for FPTree
         */

    else {
        leaf_node * leaf_neigbor = LEAF_RAW(neighbor);
        for (i = 0; i < order; i++)
        {
            if(!bitmapGet(leaf_neigbor->bitmap, i))
            {
                leaf_neigbor->keys[i] = n->keys[j];
                leaf_neigbor->pointers[i] = n->pointers[j];
                // leaf_neigbor->num_keys++;
                bitmapSet(leaf_neigbor->bitmap, i);
            }
        }
        /*for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;*/


        leaf_neigbor->pointers[order - 1] = n->pointers[order - 1];
    }

    leaf_node *leaf_n = LEAF_RAW(n);
    root = delete_entry(root, leaf_n->parent, k_prime, n);
    free(leaf_n->keys);
    free(leaf_n->pointers);
    free(leaf_n);
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 * FPTree change:
 * Probabily FIXME:
 * I think maybe we should leave the leaf nodes there even there is less than half of the space is used
 */
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index,
                          int k_prime_index, int k_prime) {

    int i;
    node * tmp;

    /* FPTree change: don't do anything
    */
    if (IS_LEAF(n))
        return root;

    /* Case: n has a neighbor to the left.
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!IS_LEAF(n))
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!IS_LEAF(n)) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

        /* Case: n is the leftmost child.
         * Take a key-pointer pair from the neighbor to the right.
         * Move the neighbor's leftmost key-pointer pair
         * to n's rightmost position.
         */

    else {
        if (IS_LEAF(n)) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!IS_LEAF(n))
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
node * delete_entry( node * root, node * n, int key, void * pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, pointer);

    /* Case:  deletion from the root.
     */

    if (n == root)
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = IS_LEAF(n) ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */
    if(!IS_LEAF(n)){
        if (n->num_keys >= min_keys)
            return root;}
    else{
        if (LEAF_RAW(n)->num_keys >= min_keys)
            return root; }

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate left neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    neighbor_index = get_neighbor_index(n);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    if(!IS_LEAF(n)){
        k_prime = n->parent->keys[k_prime_index];
        neighbor = neighbor_index == -1 ? n->parent->pointers[1] :
                   n->parent->pointers[neighbor_index];
        capacity = order - 1;
        /* Coalescence. */

        if (neighbor->num_keys + n->num_keys < capacity)
            return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);
            /* Redistribution. */
        else
            return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);} else{
        /*for leaf_nodes, only delete it after no valid keys is there*/

        if (LEAF_RAW(n)->num_keys == 0) {

            leaf_node *leaf_n = LEAF_RAW(n);
            neighbor = neighbor_index == -1 ? leaf_n->parent->pointers[1] : leaf_n->parent->pointers[neighbor_index];
            k_prime = LEAF_RAW(n)->parent->keys[k_prime_index];
            delete_entry(root, leaf_n->parent, k_prime, n);
            LEAF_RAW(neighbor)->pointers[order - 1] = leaf_n->pointers[order - 1];
            free(leaf_n->keys);
            free(leaf_n->pointers);
            free(leaf_n);
            return root;
        }else
            return root;

        /*
        k_prime = LEAF_RAW(n)->parent->keys[k_prime_index];

        capacity = order;

        if (LEAF_RAW(neighbor)->num_keys + LEAF_RAW(n)->num_keys < capacity)
            return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);
        else
            return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);*/
    }

}



/* Master deletion function.
 */
node * delete(node * root, int key) {

node * key_leaf;
record * key_record;

key_record = find(root, key, false, NULL);
key_leaf = find_leaf(root, key, false);
if (key_record != NULL && key_leaf != NULL) {
    root = delete_entry(root, key_leaf, key, key_record);
    free(key_record);
}
return root;
}


void destroy_tree_nodes(node * root) {
    int i;
    if (IS_LEAF(root)){
        /* FPTree change
        for (i = 0; i < root->num_keys; i++)
        free(root->pointers[i]);*/
        for (i = 0; i < order - 1; i++)
            if(root->pointers[i] != NULL)
                free(root->pointers[i]);
    }

    else
        for (i = 0; i < root->num_keys + 1; i++)
            destroy_tree_nodes(root->pointers[i]);
    free(root->pointers);
    free(root->keys);
    free(root);
}


node * destroy_tree(node * root) {
    destroy_tree_nodes(root);
    return NULL;
}
