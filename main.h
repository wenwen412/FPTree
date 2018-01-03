//
// Created by WenPan on 12/16/17.
//

#ifndef FPTREE_MAIN_H
#define FPTREE_MAIN_H
// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif


// Default order is 4.
#define DEFAULT_ORDER 10

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 64

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

#define MAX_KEYS (DEFAULT_ORDER-1)
typedef unsigned char byte;
#define BIT (8*sizeof(byte))
#define BITMAP_NOTFOUND -1
/**
 * Macros to manipulate pointer tags
 * Useage: node * tmp = (node*)SET_LEAF(make_leaf(xxx));
 *         if (IS_LEAF(n)) {art_leaf *l = LEAF_RAW(n);}
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void*)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((leaf_node*)((void*)((uintptr_t)x & ~1)))




#endif //FPTREE_MAIN_H

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record {
    int value;
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    // bool is_leaf; Design 2 kinds of nodes instead of using a is_leaf flag
    int num_keys;
    struct node * next; // Used for queue. FIXME: just for debug purpose, it can be removed
} node;

typedef struct leaf_node {
    int num_keys;
    // maximun num of keys is 49 here
    //7 + 8 + 49 = 64 bytes
    unsigned char bitmap[7]; // n/8
    struct leaf_node * next; //8 bytes
    unsigned  char fingerprint[49]; //1 bytes * n

    void ** pointers;
    int * keys;
    struct node * parent;
} leaf_node;






/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
// Output and utility.


void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
void enqueue( node * new_node );
node * dequeue( void );
int height( node * root );
int path_to_root( node * root, node * child );
void print_leaves( node * root );
void print_tree( node * root );
void find_and_print(node * root, int key, bool verbose);
void find_and_print_range(node * root, int range1, int range2, bool verbose);
int find_range( node * root, int key_start, int key_end, bool verbose,
                int returned_keys[], void * returned_pointers[]);
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose, record * r);
int cut( int length );

// Insertion.

record * make_record(int value);
node * make_node( bool is_leaf );
node * make_leaf( void );
int get_left_index(node * parent, node * left);
node * insert_into_leaf( node * leaf, int key, record * pointer );
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key,
                                        record * pointer);
node * insert_into_node(node * root, node * parent,
                        int left_index, int key, node * right);
node * insert_into_node_after_splitting(node * root, node * parent,
                                        int left_index,
                                        int key, node * right);
node * insert_into_parent(node * root, node * left, int key, node * right);
node * insert_into_new_root(node * left, int key, node * right);
node * start_new_tree(int key, record * pointer);
node * insert( node * root, int key, int value );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
                          int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * delete( node * root, int key );
node * destroy_tree(node * root);