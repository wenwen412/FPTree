//
// Created by WenPan on 12/30/17.
//
#include <time.h>
#include "main.h"
extern bool verbose_output;
extern  int order;
extern node * queue;
// MAIN

int main( int argc, char ** argv ) {

    char * input_file;
    FILE * fp;
    node * root;
    int input, range2;
    char instruction;
    char license_part;

    root = NULL;
    verbose_output = false;

    if (argc > 1) {
        order = atoi(argv[1]);
        if (order < MIN_ORDER || order > MAX_ORDER) {
            fprintf(stderr, "Invalid order: %d .\n\n", order);
            usage_3();
            exit(EXIT_FAILURE);
        }
    }

    license_notice();
    usage_1();
    usage_2();

    if (argc > 2) {
        input_file = argv[2];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure to open input file.");
            exit(EXIT_FAILURE);
        }
//        while (!feof(fp)) {
//            fscanf(fp, "%d\n", &input);
//            root = insert(root, input, input);
//        }

    int nums = 0;
    while (nums<=14) {
        nums++;
            //fscanf(fp, "%d\n", &input);
            input = nums;
            root = insert(root, input, input);
        }
        fclose(fp);
        print_tree(root);
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
            case 'd':
                scanf("%d", &input);
                root = delete(root, input);
                print_tree(root);
                break;
            case 'i':
                scanf("%d", &input);
                root = insert(root, input, input);
                print_tree(root);
                break;
            case 'f':
            case 'p':
                scanf("%d", &input);
                find_and_print(root, input, instruction == 'p');
                break;
            case 'r':
                scanf("%d %d", &input, &range2);
                if (input > range2) {
                    int tmp = range2;
                    range2 = input;
                    input = tmp;
                }
                find_and_print_range(root, input, range2, instruction == 'p');
                break;
            case 'l':
                print_leaves(root);
                break;
            case 'q':
                while (getchar() != (int)'\n');
                return EXIT_SUCCESS;
                break;
            case 't':
                print_tree(root);
                break;
            case 'v':
                verbose_output = !verbose_output;
                break;
            case 'x':
                if (root)
                    root = destroy_tree(root);
                print_tree(root);
                break;
            default:
                usage_2();
                break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}

/*
int test_FPTree_insert()
{
    art_tree t;
    int res = art_tree_init(&t);
    //fail_unless(res == 0, "error, res != 0");

    int len;
    char buf[512];
    FILE *f = fopen("words.txt", "r");

    uintptr_t line = 1;
    while (fgets(buf, sizeof buf, f)) {
        printf("\n",line);
        len = strlen(buf);
        buf[len-1] = '\0';
        art_insert(&t, (unsigned char*)buf, len, (void*)line);
        //fail_unless(NULL == art_insert(&t, (unsigned char*)buf, len, (void*)line));
        if (art_size(&t) != line)
            return 1;
        //fail_unless(art_size(&t) == line);
        line++;
    }

    res = art_tree_destroy(&t);
    if(!res)
        return 1;
    //fail_unless(res == 0);
    return 0;
}

int main(int argc, char **argv) {
    clock_t start, finish;
    double duration;
    start = clock();

    char * filename = argv[1];

    //test_art_insert();
    //test_art_insert_verylong();
    test_art_insert_search(filename);
    //test_art_insert_delete();
    //test_art_insert_iter();

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "%f seconds\n", duration );
}

*/