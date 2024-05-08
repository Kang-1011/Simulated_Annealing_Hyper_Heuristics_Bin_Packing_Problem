// This version pass the solution checker

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* global parameters */
int RAND_SEED[] = {1, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 330, 340, 350};
int NUM_OF_RUNS = 1;
int MAX_TIME = 30; // max amount of time permited (in sec)
int NUM_OF_PROBLEM;

int NUM_OF_LL_HEURISTICS = 4;
enum Low_Level_Heuristic
{
    Shift = 1,
    Split = 2,
    Exchange_LargestBin_LargestItem = 3,
    Exchange_RandomBin_Reshuffle = 4
};

/* parameters for simulated annealing */
float SA_TS = 300;        // starting temperature
float SA_TF = 10;        // stopping temperature
float SA_Cur_T;
float SA_Timp;
int SA_iter;
int SA_Ca;
bool SA_FR;
float SA_rs = 0.1;        // initial non-improving acceptance ratio
float SA_re = 0.005;        // stopping non-improving acceptance ratio
int SA_MAX_ITER__K = 1500000;
int SA_nrep;
float SA_w_min;     // minimal weight of each heuristic
int SA_LP;          // length of single learning period
double SA_BETA = 0.0000003;      // temperature reduction rate
int SA_DELTA;
bool SA_NEW_SLN = false;

void init_SA_parameters()
{
    // SA_rs = 0.1;
    // SA_re = 0.005;
    // SA_TS = 300;        // FINETUNE
    // SA_TF = 10;         // FINETUNE
    // printf("SA_rs=%f, SA_re=%f, SA_TS=%f, SA_TF=%f\n", SA_rs, SA_re, SA_TS, SA_TF);

    // SA_MAX_ITER__K = 1500000;
    SA_nrep = NUM_OF_LL_HEURISTICS;
    SA_w_min = (float)100 * SA_nrep / SA_MAX_ITER__K;
    SA_LP = SA_MAX_ITER__K / 500;
    // printf("SA_MAX_ITER__K=%d, SA_nrep=%d, SA_w_min=%f, SA_LP=%d\n", SA_MAX_ITER__K, SA_nrep, SA_w_min, SA_LP);

    SA_BETA = (SA_TS - SA_TF) * SA_nrep / (SA_MAX_ITER__K * SA_TF * SA_TS);
    // printf("SA_BETA=%f\n", SA_BETA);

    SA_Cur_T = SA_TS;
    SA_Timp = SA_TS;
    SA_iter = 0;
    SA_Ca = 0;
    SA_FR = false;
    // printf("SA_Cur_T=%f, SA_Timp=%f, SA_iter=%d, SA_Ca=%d, SA_FR=%d\n", SA_Cur_T, SA_Timp, SA_iter, SA_Ca, SA_FR);
} // Function test passed

struct item_struct
{
    int indx;
    int size;
};

struct problem_struct
{
    char *problem_id; // problem identifier
    int bin_capacity; // bin capacity
    int num_of_items; // number of items
    int best_obj;     // best known objective (least number of bins)
    struct item_struct *items;
};

struct bin_struct
{
    struct item_struct **stored_items; // array of pointers to items
    int cap_left;
    int num_of_items_stored;
};

struct solution_struct
{
    struct problem_struct *prob; // maintain a shallow copy of problem data
    int objective;               // number of bins used
    int feasibility;             // indicate the feasibility of the solution
    struct bin_struct *bins;
};
struct solution_struct *best_sln;
struct solution_struct *candidate_sln;
struct solution_struct *SA_cur_sln;

struct heuristics_struct
{
    int index;
    int c_accept;
    int c_new;
    int c_total;
    float weight;
};

// return a random number between 0 and 1
float rand_01()
{
    float number;
    number = (float)rand();
    number = number / RAND_MAX;
    // printf("rand01=%f\n", number);
    return number;
} // Function test passed

// return a random number ranging from min to max (inclusive)
int rand_int(int min, int max)
{
    int div = max - min + 1;
    int val = rand() % div + min;
    // printf("rand_range= %d \n", val);
    return val;
} // Function test passed

// Initialise a problem instance
void init_problem(char problem_id[], int bin_capacity, int num_of_items, int best_obj, struct problem_struct **my_prob)
{
    struct problem_struct *new_prob = malloc(sizeof(struct problem_struct));

    // Dynamically allocate memory for problem_id and copy the string
    new_prob->problem_id = malloc(strlen(problem_id) + 1); // +1 for null terminator
    strcpy(new_prob->problem_id, problem_id);

    new_prob->bin_capacity = bin_capacity;
    new_prob->num_of_items = num_of_items;
    new_prob->best_obj = best_obj;

    new_prob->items = malloc(sizeof(struct item_struct) * num_of_items);
    *my_prob = new_prob;
} // Function test passed

struct problem_struct **load_problems(char *data_file)
{
    FILE *fp = fopen(data_file, "r");
    if (fp == NULL)
    {
        printf("Data file %s does not exist. Please check!\n", data_file);
        exit(2);
    }

    fscanf(fp, "%d", &NUM_OF_PROBLEM); // Read the number of test problems (P) in this file

    // Allocate memory for the pointer of pointers to {struct problem_struct}
    // This pointer keeps a list of references to all the problem instance in this file
    struct problem_struct **my_problems = malloc(sizeof(struct problem_struct *) * NUM_OF_PROBLEM);

    for (int i = 0; i < NUM_OF_PROBLEM; i++) // Every iteration is a problem instance
    {
        char string[10];
        fscanf(fp, " %s", string); // Read the problem identifier

        int cap, num, obj;
        fscanf(fp, "%d", &cap); // Read the bin capacity in this problem
        fscanf(fp, "%d", &num); // Read the number of items(n) in this problem
        fscanf(fp, "%d", &obj); // Read the best objective of this problem (least number of bins)

        init_problem(string, cap, num, obj, &my_problems[i]);

        // Read item-specific data of this problem
        for (int j = 0; j < num; j++)
        {
            my_problems[i]->items[j].indx = j;
            fscanf(fp, "%d", &(my_problems[i]->items[j].size));
        }
    }
    fclose(fp);

    return my_problems;
} // Function test passed

// Free memory in problems
void free_problem(struct problem_struct *prob)
{
    if (prob != NULL)
    {
        if (prob->problem_id != NULL)
        {
            free(prob->problem_id);
        }

        if (prob->items != NULL)
        {
            free(prob->items);
        }
        free(prob);
    }
} // Function test passed

// Test function for problem input
void print_problem(struct problem_struct **problems)
{
    for (int i = 0; i < NUM_OF_PROBLEM; i++)
    {
        printf("In print_problems: %s\n", problems[i]->problem_id);
        printf("%d %d %d\n", problems[i]->bin_capacity, problems[i]->num_of_items, problems[i]->best_obj);

        if (i != -1)
        {
            for (int j = 0; j < problems[i]->num_of_items; j++)
            {
                printf("%d %d\n", problems[i]->items[j].indx, problems[i]->items[j].size);
            }
        }
    }
} // Function test passed

struct solution_struct *init_solution(struct problem_struct *prob)
{
    struct solution_struct *sln = malloc(sizeof(struct solution_struct));
    sln->prob = prob;
    sln->objective = 0;
    sln->feasibility = false;
    sln->bins = malloc(sizeof(struct bin_struct));

    return sln;
} // Function test passed

// Add a new empty bin into the solution
struct solution_struct *add_bin(struct solution_struct *sln)
{
    sln->bins = realloc(sln->bins, (sln->objective + 1) * sizeof(struct bin_struct));

    if (sln->bins == NULL)
    {
        // Handle error
        printf("Memory allocation when adding bin for solution %s failed!\n", sln->prob->problem_id);
        exit(1); // Exit the program
    }

    sln->bins[sln->objective].stored_items = malloc(sizeof(struct item_struct *));
    sln->bins[sln->objective].num_of_items_stored = 0;
    sln->bins[sln->objective].cap_left = sln->prob->bin_capacity;

    sln->objective++;

    return sln;
} // Function test passed

struct solution_struct *delete_bin(struct solution_struct *sln, int bin_no)
{
    if ((sln->bins[bin_no].num_of_items_stored != 0) || (sln->bins[bin_no].cap_left != sln->prob->bin_capacity))
    {
        printf("Bin %d is not empty, cannot be deleted!\n", bin_no);
    }
    else 
    {
        printf("Starting to delete bin %d\n", bin_no);
        // Shift bins' pointers to fill the gap
        for (int i = bin_no; i < sln->objective - 1; i++)
        {
            if (i == sln->objective - 1)
            {
                printf("Deleted bin %d\n", bin_no);
                break;
            }

            sln->bins[i] = sln->bins[i + 1];
        }
        sln->objective--;
    }
    
    return sln;
} // Function test passed

struct solution_struct add_item_to_bin(struct solution_struct *sln, int bin_no, struct item_struct *item)
{
    sln->bins[bin_no].stored_items = realloc(sln->bins[bin_no].stored_items, (sln->bins[bin_no].num_of_items_stored + 1) * sizeof(struct item_struct *));
    printf("Adding item %d to bin %d\n", item->indx, bin_no);
    sln->bins[bin_no].stored_items[sln->bins[bin_no].num_of_items_stored] = item;

    sln->bins[bin_no].cap_left -= item->size;
    sln->bins[bin_no].num_of_items_stored++;

    //printf("Item %d added to bin %d\n", item->indx, bin_no);

    return *sln;
} // Function test passed

struct solution_struct *delete_item_from_bin(struct solution_struct *sln, int bin_no, int item_index)
{
    if (item_index < 0 || item_index >= sln->bins[bin_no].num_of_items_stored) {
        printf("Invalid item index in delete_item_from_bin\n");
        return NULL;
    }

    int removed_item_size = sln->bins[bin_no].stored_items[item_index]->size;

    // Shift item's pointers to fill the gap
    for (int i = item_index; i < sln->bins[bin_no].num_of_items_stored; i++) {
        
        if (i == sln->bins[bin_no].num_of_items_stored - 1)
        {
            printf("Deleted item %d from bin %d\n", i, bin_no);
            break;
        }
        sln->bins[bin_no].stored_items[i] = sln->bins[bin_no].stored_items[i + 1];
    }

    // Update bin info
    sln->bins[bin_no].num_of_items_stored--;
    sln->bins[bin_no].cap_left += removed_item_size;

    return sln;
} // Function test passed

struct solution_struct *item_change_bin(struct solution_struct *sln, struct item_struct *item, int item_src_index, int src_bin_no, int dst_bin_no)
{
    delete_item_from_bin(sln, src_bin_no, item_src_index);
    add_item_to_bin(sln, dst_bin_no, item);
    //printf("TESTING: %d\n", sln->bins[src_bin_no].stored_items[1]->indx);
} // Function test passed

int cmp_item_size_decreasing(const void *a, const void *b)
{
    const struct item_struct *item1 = a;
    const struct item_struct *item2 = b;
    if (item1->size > item2->size)
        return -1;
    if (item1->size < item2->size)
        return 1;
    return 0;
} // Function test passed

int cmp_item_index_increasing(const void *a, const void *b)
{
    const struct item_struct *item1 = a;
    const struct item_struct *item2 = b;
    if (item1->indx > item2->indx)
        return 1;
    if (item1->indx < item2->indx)
        return -1;
    return 0;
} // Function test passed

struct solution_struct *first_fit(struct problem_struct *prob)
{
    struct solution_struct *instance_solution = init_solution(prob);
    printf("\nSolution initiated for problem %s\n", prob->problem_id);

    int k;
    for (int j = 0; j < prob->num_of_items; j++)
    {
        // printf("Enter checking for item %d\n", prob->items[j].indx);
        for (k = 0; k < instance_solution->objective; k++)
        {
            if (instance_solution->bins[k].cap_left >= prob->items[j].size)
            {
                add_item_to_bin(instance_solution, k, &prob->items[j]);
                break;
            }
        }
        if (k == instance_solution->objective)
        {
            add_bin(instance_solution);

            add_item_to_bin(instance_solution, instance_solution->objective - 1, &prob->items[j]);
        }
    }

    return instance_solution;
} // Function test passed

struct solution_struct *first_fit_decreasing(struct problem_struct *prob)
{
    qsort(prob->items, prob->num_of_items, sizeof(struct item_struct), cmp_item_size_decreasing);

    return first_fit(prob);
} // Function test passed

void free_solution(struct solution_struct *sln)
{
    if (sln != NULL)
    {
        if (sln->bins != NULL)
        {
            for (int i = 0; i < sln->objective; i++)
            {
                if (sln->bins[i].stored_items != NULL)
                {
                    free(sln->bins[i].stored_items);
                }
            }
            free(sln->bins);
            printf("Bins freed\n");
        }

        sln->prob = NULL;
        sln->objective = 0;
        sln->feasibility = false;

        free(sln);
    }
} // Function test passed

void output_solution(struct solution_struct *sln, char *output_file, int flag)
{
    if (output_file != NULL)
    {
        FILE *pfile;
        if (flag == 0)
        {
            pfile = fopen(output_file, "w"); // append solution data
            fprintf(pfile, "%d\n", NUM_OF_PROBLEM);
        }
        else
        {
            pfile = fopen(output_file, "a"); // append solution data
        }

        fprintf(pfile, "%s\n", sln->prob->problem_id);
        fprintf(pfile, " obj=   %i \t %d\n", sln->objective, sln->objective - sln->prob->best_obj);

        for (int i = 0; i < sln->objective; i++)
        {
            for (int j = 0; j < sln->bins[i].num_of_items_stored; j++)
            {
                fprintf(pfile, "%d ", sln->bins[i].stored_items[j]->indx);
            }
            fprintf(pfile, "\n");
        }

        fclose(pfile);
    }
    else
        printf("sln.feas=%d, sln.obj=%d\n", sln->feasibility, sln->objective);
} // Function test passed

void test_output_solution(struct solution_struct *sln, char *output_file, int flag)
{
    if (output_file != NULL)
    {
        FILE *pfile;
        if (flag == 0)
        {
            pfile = fopen(output_file, "w"); // append solution data
            fprintf(pfile, "%d\n", NUM_OF_PROBLEM);
        }
        else
        {
            pfile = fopen(output_file, "a"); // append solution data
        }

        fprintf(pfile, "%s\n", sln->prob->problem_id);
        fprintf(pfile, " obj=   %i \t %d\n", sln->objective, sln->objective - sln->prob->best_obj);

        for (int i = 0; i < sln->objective; i++)
        {
            for (int j = 0; j < sln->bins[i].num_of_items_stored; j++)
            {
                fprintf(pfile, "%d(%d) ", sln->bins[i].stored_items[j]->indx, sln->bins[i].stored_items[j]->size);
            }
            fprintf(pfile, "\n");
        }

        fclose(pfile);
    }
    else
        printf("sln.feas=%d, sln.obj=%d\n", sln->feasibility, sln->objective);
} // Function test passed

struct solution_struct *copy_solution(struct solution_struct *src_sln, struct solution_struct *dst_sln)
{
    if (src_sln == NULL)
    {
        printf("Source solution is NULL in copy_solution\n");
        return NULL; // modify this code to return a proper error code
    }
    if (dst_sln == NULL)
    {   
        printf("Destination solution is NULL in copy_solution\n");
    }
    else
    {
        //free_solution(dst_sln);
        printf("Destination solution is not NULL in copy_solution\n");
    }
    dst_sln = malloc(sizeof(struct solution_struct));
    dst_sln->prob = src_sln->prob;
    dst_sln->objective = src_sln->objective;
    dst_sln->feasibility = src_sln->feasibility;

    printf("Starting to copy bins\n");
    int num_of_bins = src_sln->objective;
    dst_sln->bins = malloc(sizeof(struct bin_struct) * num_of_bins);
    for (int i = 0; i < num_of_bins; i++)
    {
        dst_sln->bins[i].stored_items = malloc(sizeof(struct item_struct *) * src_sln->bins[i].num_of_items_stored);
        for (int j = 0; j < src_sln->bins[i].num_of_items_stored; j++)
        {
            dst_sln->bins[i].stored_items[j] = src_sln->bins[i].stored_items[j];
        }
        
        dst_sln->bins[i].cap_left = src_sln->bins[i].cap_left;
        dst_sln->bins[i].num_of_items_stored = src_sln->bins[i].num_of_items_stored;
    }
    printf("Done copying bins\n");

    return dst_sln;
} // Function test passed

// update global best solution from sln
void update_best_solution(struct solution_struct *sln)
{
    if (best_sln == NULL)
    {
        printf("Best solution is NULL in update_best_solution\n");
        best_sln = malloc(sizeof(struct solution_struct));
        best_sln->objective = 0;
        best_sln->feasibility = false;
    }
    if (best_sln->objective < sln->objective)
    {
        printf("Copying to best solution\n");
        best_sln = copy_solution(sln, best_sln);
        printf("Finish copying to best solution\n");
    }
} // Function test passed

// update neighbouring solution from sln
void update_candidate_solution(struct solution_struct *sln)
{
    if (candidate_sln == NULL)
    {
        printf("Candidate solution is NULL in update_candidate_solution\n");
        candidate_sln = malloc(sizeof(struct solution_struct));
        candidate_sln->objective = 0;
        candidate_sln->feasibility = false;
    }
    printf("Copying to candidate solution\n");
    candidate_sln = copy_solution(sln, candidate_sln);
    printf("Finish copying to candidate solution\n");
} // Function test passed

// update current solution from sln
void update_SA_current_solution(struct solution_struct *sln)
{
    if (SA_cur_sln == NULL)
    {
        printf("SA's current solution is NULL in update_SA_current_solution\n");
        SA_cur_sln = malloc(sizeof(struct solution_struct));
        SA_cur_sln->objective = 0;
        SA_cur_sln->feasibility = false;
    }
    printf("Copying to SA's current solution\n");
    SA_cur_sln = copy_solution(sln, SA_cur_sln);
    printf("Finish copying to SA's current solution\n");
} // Function test passed

int get_largest_residual_bin(struct solution_struct *sln)
{
    int max_residual_capacity = 0;          // largest residual capacity
    int largest_residual_bin_index = -1;     // bin index of the bin with the largest residual capacity

    // Find the bin with the largest residual capacity
    for (int i = 0; i < sln->objective; i++)
    {
        if (sln->bins[i].cap_left > max_residual_capacity)
        {
            max_residual_capacity = sln->bins[i].cap_left;
            largest_residual_bin_index = i;
        }
        //printf("Residual capacity for bin %d: %d\n", i, sln->bins[i].cap_left);
    }
    //printf("Largest residual bin index: %d\n", largest_residual_bin_index);

    return largest_residual_bin_index;
} // Function test passed

struct solution_struct *shift_h1(struct solution_struct *current_solution)
{
    printf("Starting Heuristic: shift_h1\n");
    int largest_residual_bin_index = get_largest_residual_bin(current_solution); // index of the bin with the largest residual capacity

    for (int i = 0; i < current_solution->bins[largest_residual_bin_index].num_of_items_stored; i++)
    {
        int best_bin_no = -1;
        int best_capacity = current_solution->prob->bin_capacity;
        for (int bin_index = 0; bin_index < current_solution->objective; bin_index++)
        {
            if (bin_index == largest_residual_bin_index)
            {
                continue;
            }

            if ((current_solution->bins[bin_index].cap_left >= current_solution->bins[largest_residual_bin_index].stored_items[i]->size && current_solution->bins[bin_index].cap_left < best_capacity) || (current_solution->bins[bin_index].cap_left >= current_solution->bins[largest_residual_bin_index].stored_items[i]->size && current_solution->prob->bin_capacity == best_capacity))
            {
                best_bin_no = bin_index;
                best_capacity = current_solution->bins[bin_index].cap_left;
            }
        }
        
        if (best_bin_no == -1)
        {
        }
        else
        {
            item_change_bin(current_solution, current_solution->bins[largest_residual_bin_index].stored_items[i], i, largest_residual_bin_index, best_bin_no);
        }
        
    }
    printf("Candidate Objective: %d\n", current_solution->objective);
    printf("Finished Heuristic: shift_h1\n");
    return current_solution;
} // Not tested yet - DEBUG

struct solution_struct *split_h2(struct solution_struct *current_solution)
{/*
    printf("Starting Heuristic: split_h2\n");
    // Calculate average load
    int total_load = 0;
    for (int i = 0; i < current_solution->objective; i++) {
        total_load += current_solution->bins[i].num_of_items_stored;
    }
    float average_load = total_load / current_solution->objective;

    int current_bin_num = current_solution->objective;

    // Iterate through bins
    for (int i = 0; i < current_bin_num; i++) {
        // Bin with index i
        if (current_solution->bins[i].num_of_items_stored > average_load) {
            // Create a new bin
            add_bin(current_solution);
            
            // Move half of items to the new bin
            int half_num = current_solution->bins[i].num_of_items_stored / 2;
            for (int j = 0; j < half_num; j++) {
                // Randomly select an item
                int random_item_index = rand_int(0, current_solution->bins[i].num_of_items_stored - 1);
                item_change_bin(current_solution, current_solution->bins[i].stored_items[random_item_index], random_item_index, i, current_solution->objective - 1);
            }
        }

        if (i > current_bin_num * 2){
            printf("Error in split_h2: i > current_bin_num * 2\n");
            break;
        }
    }
    printf("Finished Heuristic: split_h2\n");
    return current_solution;*/
} // Not tested yet - DEBUG

int get_largest_item(struct solution_struct *sln, int bin_index)
{
    int max_item_size = 0;       // largest item size
    int largest_item_index = -1;      // index of the item with the largest size

    // Find the item with the largest size in the bin with the largest residual capacity
    for (int i = 0; i < sln->bins[bin_index].num_of_items_stored; i++)
    {
        if (sln->bins[bin_index].stored_items[i]->size > max_item_size)
        {
            max_item_size = sln->bins[bin_index].stored_items[i]->size;
            largest_item_index = i;
        }
    }
    //printf("Largest item index: %d\n", largest_item_index);

    return largest_item_index;
} // Function test passed

struct solution_struct *exchange_largest_bin_largest_item(struct solution_struct *current_solution)
{/*
    printf("Starting Heuristic: exchange_largest_bin_largest_item\n");
    int largest_residual_bin_index = get_largest_residual_bin(current_solution); // index of the bin with the largest residual capacity

    int max_item_index = get_largest_item(current_solution, largest_residual_bin_index); // index of the item with the largest size
    int exchange_capacity = current_solution->bins[largest_residual_bin_index].stored_items[max_item_index]->size; // remaining capacity available to exchange

    int random_bin_index = -1;
    int exchange_item_index = -1;

    // Randomly select a non-fully-filled bin    
    do
    {
        random_bin_index = rand_int(0, current_solution->objective - 1);    // randomly select a bin index

        // check if the bin is full
        if (current_solution->bins[random_bin_index].cap_left <= 0 || random_bin_index == largest_residual_bin_index)
        {
            continue;
        }

        // loop through the bin to check if the bin contains item smaller than the exchange capacity
        for (int i = 0; i < current_solution->bins[random_bin_index].num_of_items_stored; i++)
        {
            if (current_solution->bins[random_bin_index].stored_items[i]->size < exchange_capacity)
            {
                exchange_item_index = i;
                break;
            }
        }
    } while (exchange_item_index == -1);
    printf("Random bin index: %d\n", random_bin_index);

    // If the bin contains an item smaller than the exchange capacity, exchange it
    if (exchange_item_index != -1)
    {
        if (current_solution->bins[random_bin_index].stored_items[exchange_item_index]->size < exchange_capacity)
        {
            struct item_struct *exchange_item = current_solution->bins[random_bin_index].stored_items[exchange_item_index];
            struct item_struct *max_item = current_solution->bins[largest_residual_bin_index].stored_items[max_item_index];

            current_solution = item_change_bin(current_solution, max_item, max_item_index, largest_residual_bin_index, random_bin_index);
            current_solution = item_change_bin(current_solution, exchange_item, exchange_item_index, random_bin_index, largest_residual_bin_index);

            exchange_capacity -= current_solution->bins[random_bin_index].stored_items[exchange_item_index]->size;
        }
    }

    // Continue transfering items from the randomly selected bin to the bin with the largest residual capacity if the capacity sum is less than the exchange capacity
    for (int i = 0; i < current_solution->bins[random_bin_index].num_of_items_stored; i++)
    {
        if (current_solution->bins[random_bin_index].stored_items[i]->size < exchange_capacity)
        {
            struct item_struct *exchange_item = current_solution->bins[random_bin_index].stored_items[i];

            current_solution = item_change_bin(current_solution, exchange_item, i, random_bin_index, largest_residual_bin_index);

            exchange_capacity -= current_solution->bins[random_bin_index].stored_items[i]->size;
        }
    }  

    printf("Finished Heuristic: exchange_largest_bin_largest_item\n");
    printf("Current solution: %d\n", current_solution->objective);
    
    return current_solution;*/
} // Function test passed

int select_non_fully_filled_bin_probabilistically(struct solution_struct *sln) {
    // Calculate total residual capacity
    int total_residual_capacity = 0;
    for (int i = 0; i < sln->objective; i++) {
        total_residual_capacity += sln->bins[i].cap_left;
    }
    
    int random_number = rand() % total_residual_capacity;
    int cumulative_capacity = 0;
    for (int i = 0; i < sln->objective; i++) {
        cumulative_capacity += sln->bins[i].cap_left;
        //printf("Capacity[%d]=%d, Cumulative=%d, Random=%d\n", i, bins[i], cumulative_capacity, random_number);
        if (random_number <= cumulative_capacity) {
            // printf("Selected bin: %d\n", i);
            return i;
        }
    }
    return -1; // Return -1 if no item is selected
} // Function test passed

void int_to_bits(int num, int* bitArray, int size) {
    int i;
    for (i = size - 1; i >= 0; i--) {
        bitArray[i] = (num >> i) & 1;
    }
}

int *items_permutation(struct item_struct *item_storage, int item_count, int bin_capacity, int min_residual_capacity)
{
    printf("Finding best packing...\n");
    /*if (item_count > 8)
    {   
        // TODO
        // Form a new array of size 8
        printf("-----Too much items, randomly select two bins again-----\n");
        return exchange_random_bin_reshuffle(sln);
    }*/

    int bit_length;
    int max_num;

    /*if (item_count > 8)
    {
        bit_length = 8;
    }
    else
    {
        bit_length = item_count;
    }*/
    bit_length = item_count;
    max_num = pow(2, bit_length);
    //printf("Bit length: %d, max num: %d\n", bit_length, max_num);

    int *best_array = (int*)malloc(bit_length * sizeof(int));
    
    for (int number = 0; number < max_num && number < 600; number++)
    {
        //printf("Converting int to bits\n");
        int bit_array[bit_length];
        int_to_bits(number, bit_array, bit_length);

        int remaining_capacity = bin_capacity;
        for (int bit_index = 0; bit_index < bit_length; bit_index++) {
            //printf("Bit %d: %d\n", bit_index, bit_array[bit_index]);
            if (bit_array[bit_index] == 1)
            {
                //printf("Size of item %d: %d\n", bit_index, item_storage[bit_index].size);
                remaining_capacity -= item_storage[bit_index].size;
            }
            if (remaining_capacity < 0)
            {
                break;
            }
        }
        if ((remaining_capacity <= min_residual_capacity) && (remaining_capacity >= 0))
        {
            min_residual_capacity = remaining_capacity;

            for (int x = 0; x < bit_length; x++) {
                best_array[x] = bit_array[x];
            }
        }
    }
    
    printf("Best array computed\n");
    return best_array;
}

struct solution_struct *two_bins_best_pack(struct solution_struct *current_solution, int bin1_index, int bin2_index)
{
    int bin_index[] = {bin1_index, bin2_index};
    int bin_num_of_items[] = {current_solution->bins[bin1_index].num_of_items_stored, current_solution->bins[bin2_index].num_of_items_stored};
    // printf("Bin %d num of items: %d, Bin %d num of items: %d\n", bin_index[0], bin_num_of_items[0], bin_index[1], bin_num_of_items[1]);

    int items_amount = current_solution->bins[bin1_index].num_of_items_stored + current_solution->bins[bin2_index].num_of_items_stored;
    struct item_struct *temp_item_storage = malloc(sizeof(struct item_struct) * items_amount); // Array of references to all items from both bins

    printf("Bin1 index: %d, Bin2 index: %d\n", bin1_index, bin2_index);
    int item_count = 0;

    // Initialize the array and sort them by item size in descending order
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < current_solution->bins[bin_index[i]].num_of_items_stored; j++)
        {
            temp_item_storage[item_count] = *current_solution->bins[bin_index[i]].stored_items[j];
            item_count++;
        }
    }
    
    // Find the best packing
    int min_res_cap = fmin(current_solution->bins[bin1_index].cap_left, current_solution->bins[bin2_index].cap_left);
    int *best_packing = items_permutation(temp_item_storage, items_amount, current_solution->prob->bin_capacity, min_res_cap);

    // Remove all items from both bins
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < bin_num_of_items[i]; j++)
        {
            delete_item_from_bin(current_solution, bin_index[i], 0);
        }
    }
    
    if (best_packing == NULL)
    {
        printf("Better packing not found, no reshuffle\n");
    }
    else
    {
        for (int j = 0; j < items_amount; j++)
        {
            if (best_packing[j] == 0)
            {
                for (int i = 0; i < current_solution->prob->num_of_items; i++)
                {
                    if (temp_item_storage[j].indx == current_solution->prob->items[i].indx)
                    {
                        add_item_to_bin(current_solution, bin1_index, &current_solution->prob->items[i]);
                        printf("Successfully added item %d to bin1 %d\n", current_solution->prob->items[i].indx, bin1_index);
                    }
                }
            }
            else if (best_packing[j] == 1)
            {
                for (int i = 0; i < current_solution->prob->num_of_items; i++)
                {
                    if (temp_item_storage[j].indx == current_solution->prob->items[i].indx)
                    {
                        add_item_to_bin(current_solution, bin2_index, &current_solution->prob->items[i]);
                        printf("Successfully added item %d to bin2 %d\n", current_solution->prob->items[i].indx, bin2_index);
                    }
                }
            }
        }
    }
    
    if (best_packing != NULL)
    {
        free(best_packing);
    }
    if (temp_item_storage != NULL)
    {
        free(temp_item_storage);
    }
    return current_solution;
} // Function test passed

struct solution_struct *exchange_random_bin_reshuffle(struct solution_struct *current_solution)
{
    printf("Starting Heuistic: exchange_random_bin_reshuffle\n");
    int random_bin1_index = -1;
    int random_bin2_index = -1;
    // Randomly select two distinct non-fully-filled bins
    do {
        random_bin1_index = select_non_fully_filled_bin_probabilistically(current_solution);
    } while (random_bin1_index == -1 || current_solution->bins[random_bin1_index].cap_left <= 0);
    do
    {
        // random_bin1_index = select_non_fully_filled_bin_probabilistically(current_solution);
        random_bin2_index = select_non_fully_filled_bin_probabilistically(current_solution);
    } while (random_bin2_index == -1 || current_solution->bins[random_bin2_index].cap_left <= 0 || (random_bin1_index == random_bin2_index));

    two_bins_best_pack(current_solution, random_bin1_index, random_bin2_index);

    printf("Ending Heuistic: exchange_random_bin_reshuffle\n");
    return current_solution;
} // Function test passed

struct heuristics_struct **initiate_heuristics()
{
    struct heuristics_struct **heuristics = malloc(sizeof(struct heuristics_struct *) * NUM_OF_LL_HEURISTICS);
    
    for (int i = 0; i < NUM_OF_LL_HEURISTICS; i++)
    {
        heuristics[i] = malloc(sizeof(struct heuristics_struct));
        heuristics[i]->index = i;
        heuristics[i]->c_accept = 0;
        heuristics[i]->c_new = 0;
        heuristics[i]->c_total = 0;
        heuristics[i]->weight = SA_w_min;

        //printf("index=%d, c_accept=%d, c_new=%d, c_total=%d, weight=%f\n", heuristics[i]->index, heuristics[i]->c_accept, heuristics[i]->c_new, heuristics[i]->c_total, heuristics[i]->weight);
    }

    return heuristics;
} // Function test passed

struct heuristics_struct *select_heuristic_probabilistically(struct heuristics_struct **heuristics) {
    // Calculate total weight
    float total_weight = 0.0;
    for (int i = 0; i < NUM_OF_LL_HEURISTICS; i++) {
        total_weight += heuristics[i]->weight;
    }
    
    float random_number = (float)rand() / RAND_MAX * total_weight;
    float cumulative_weight = 0.0;
    for (int i = 0; i < NUM_OF_LL_HEURISTICS; i++) {
        cumulative_weight += heuristics[i]->weight;
        // printf("Weight[%d]=%f, Cumulative=%f, Random=%f\n", i, heuristics[i]->weight, cumulative_weight, random_number);
        if (random_number <= cumulative_weight) {
            // printf("Selected heuristic: %d\n", i);
            return heuristics[i];
        }
    }
    return NULL; // Return null character if no item is selected
} // Function test passed

bool compare_solution(struct solution_struct *sln1, struct solution_struct *sln2)
{
    if (sln1->prob != sln2->prob)
    {
        return false;
    }

    if (sln1->objective != sln2->objective)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < sln1->objective; i++)
        {
            if (sln1->bins[i].num_of_items_stored != sln2->bins[i].num_of_items_stored)
            {
                return false;
            }
            if (sln1->bins[i].cap_left != sln2->bins[i].cap_left)
            {
                return false;
            }
        }
    }
    // printf("++Solution comparison: %d++\n", true);
    return true;
} // Function test passed

struct solution_struct *clear_empty_bin(struct solution_struct *sln)
{
    printf("Checking empty bins...\n");
    for (int i = 0; i < sln->objective; i++)
    {
        if (sln->bins[i].num_of_items_stored == 0)
        {
            delete_bin(sln, i);
        }
    }
    return sln;
} // Not tested

struct solution_struct *heuristic_search(struct solution_struct *candidate_sln, int selected_heuristic_index)
{
    switch(selected_heuristic_index)
    {
        case 0:
            printf("\nRunning Low-level Heuristic: Shift...\n");
            candidate_sln = shift_h1(candidate_sln);
            break;
        case 1:
            printf("\nRunning Low-level Heuristic: Split...\n");
            candidate_sln = split_h2(candidate_sln);
            break;
        case 2:
            printf("\nRunning Low-level Heuristic: Exchange largestBin_largestItem...\n");
            candidate_sln = exchange_largest_bin_largest_item(candidate_sln);
            break;
        case 3:
            printf("\nRunning Low-level Heuristic: Exchange randomBin_reshuffle...\n");
            candidate_sln = exchange_random_bin_reshuffle(candidate_sln);
            break;
        default:
            printf("\nNo Low-level Heuristic Selected\n");
            return NULL;
    }
    //output_solution(candidate_sln, "output.txt", 1);
    printf("++Candidate Objective: %d\n", candidate_sln->objective);
    printf("Clearing empty bins...\n");
    // clear_empty_bin(candidate_sln);
    printf("--Candidate Objective: %d\n", candidate_sln->objective);
    printf("Cleared empty bins\n");

    return candidate_sln;
} // Not tested

void learn(struct heuristics_struct **heuristics, int index, struct solution_struct *current_sln, struct solution_struct *candidate_sln, float rand01)
{
    printf("Learning...\n");
    SA_FR = false;
    heuristics[index]->c_total++;

    // If the candidate solution is modified but not accepted
    if (SA_NEW_SLN == true)
    {
        heuristics[index]->c_new++;
    }
    if (SA_DELTA < 0)
    {
        heuristics[index]->c_accept++;
        SA_Ca++;
        SA_Timp = SA_Cur_T;
        SA_FR = false;
    }
    if (SA_DELTA > 0 && exp(-SA_DELTA / SA_Cur_T) > rand01)
    {
        heuristics[index]->c_accept++;
        SA_Ca++;
    }
    if (SA_iter % SA_LP == 0)
    {
        if (SA_Ca / SA_LP < SA_re)
        {
            SA_FR = true;
                        
            printf("Temperature Reduction: %f\n", SA_BETA);
            printf("Temperature: %f, Timp: %f\n", SA_Cur_T, SA_Timp);
            SA_Timp = SA_Timp / (1 - SA_BETA * SA_Timp);
            printf("Temperature = Timp = %f\n", SA_Timp);
            SA_Cur_T = SA_Timp;
            // copy_solution(best_sln, current_sln);
            update_SA_current_solution(best_sln);

            for (int i = 0; i < NUM_OF_LL_HEURISTICS; i++)
            {
                if (heuristics[i]->c_total == 0)
                {
                    heuristics[i]->weight = SA_w_min;
                }
                else
                {
                    heuristics[i]->weight = fmax(SA_w_min, heuristics[i]->c_new / heuristics[i]->c_total);
                }
                heuristics[i]->c_accept = 0;
                heuristics[i]->c_new = 0;
                heuristics[i]->c_total = 0;
            }
        }
        else
        {
            for (int i = 0; i < NUM_OF_LL_HEURISTICS; i++)
            {
                if (heuristics[i]->c_total == 0)
                {
                    heuristics[i]->weight = SA_w_min;
                }
                else
                {
                    heuristics[i]->weight = fmax(SA_w_min, heuristics[i]->c_accept / heuristics[i]->c_total);
                }
                heuristics[i]->c_accept = 0;
                heuristics[i]->c_new = 0;
                heuristics[i]->c_total = 0;
            }
        }
        SA_Ca = 0;       
    }
}

void SimulatedAnnealing(struct solution_struct *current_sln)
{
    printf("\nRunning Simulated Annealing...\n");
    update_SA_current_solution(current_sln);

    init_SA_parameters();
    clock_t time_start, time_end;
    time_start = clock();
    double time_spent = 0;

    struct heuristics_struct **my_heuristics = initiate_heuristics();
    while (SA_iter < SA_MAX_ITER__K && time_spent < MAX_TIME && SA_Cur_T > SA_TF)
    {
        printf("\nCurrent Iteration: %d\n", SA_iter);
        SA_NEW_SLN = false;

        // Generate a copy of current solution
        update_candidate_solution(SA_cur_sln);
        printf("Candidate obj: %d, Current obj: %d\n", candidate_sln->objective, SA_cur_sln->objective);
        printf("Best obj: %d, Current obj: %d\n", best_sln->objective, SA_cur_sln->objective);

        // Select a heuristic and generate the candidate solution
        struct heuristics_struct *selected_heuristic = select_heuristic_probabilistically(my_heuristics);
        candidate_sln = heuristic_search(candidate_sln, selected_heuristic->index);
        printf("Candidate obj: %d, Current obj: %d\n", candidate_sln->objective, SA_cur_sln->objective);
        printf("Candidate solution returned\n");

        // Difference between the objective value of current solution and neighbouring solution
        SA_DELTA = candidate_sln->objective - SA_cur_sln->objective;
        SA_iter++;
        printf("Checkpoint before SA evaluation function\n");
        if (!compare_solution(SA_cur_sln, candidate_sln))
        {   
            printf("New Solution Generated\n");
            SA_NEW_SLN = true;
        }

        if (SA_DELTA <= 0 && !compare_solution(SA_cur_sln, candidate_sln))
        {
            printf("Better Solution Accepted\n");
            //copy_solution(candidate_sln, current_sln); // Replace current solution with candidate solution
            update_SA_current_solution(candidate_sln);
            update_best_solution(current_sln);
        }
        float rand01 = rand_01();
        if (SA_DELTA > 0 && exp(-SA_DELTA / SA_Cur_T) > rand01)
        {
            printf("Worse Solution but accepted\n");
            printf("--Exp: %f, Random Num: %f\n", exp(-SA_DELTA / SA_Cur_T), rand01);
            //copy_solution(candidate_sln, current_sln); // Replace current solution with candidate solution
            update_SA_current_solution(candidate_sln);
        }
        printf("Checkpoint after SA evaluation function\n");
        if (SA_FR == true)
        {
            printf("Reheating Started\n");
            SA_Timp = SA_Timp / (1 - SA_BETA * SA_Timp);
            SA_Cur_T = SA_Timp;
        }
        else if (SA_iter % SA_nrep == 0)
        {
            printf("Cooling Started\n");
            printf("Temperature before cooling: %f\n", SA_Cur_T);
            printf("Temperature reduction rate: %lf\n", SA_BETA);
            SA_Cur_T = SA_Cur_T / (1 + SA_BETA * SA_Cur_T);
            printf("Temperature after cooling: %f\n", SA_Cur_T);

            if (SA_iter % 100 == 0)
            {
                printf("Current Tempereature = %f, Current Objective = %d,\t Best Objective = %d\n", SA_Cur_T, current_sln->objective, best_sln->objective);
            }
        }
        learn(my_heuristics, selected_heuristic->index, SA_cur_sln, candidate_sln, rand01);

        time_end = clock();
        time_spent = (double)(time_end - time_start) / CLOCKS_PER_SEC;

        printf("Time spent: %f, MaxTime: %d, Current temperature: %f, Stopping temperature: %f\n", time_spent, MAX_TIME, SA_Cur_T, SA_TF);
    }
    printf("\nFinished Simulated Annealing\n");
    printf("Time spent: %f, MaxTime: %d, Current temperature: %f, Stopping temperature: %f\n", time_spent, MAX_TIME, SA_Cur_T, SA_TF);
}

struct problem_struct *copy_problem(struct problem_struct *prob)
{
    struct problem_struct *copy_prob = malloc(sizeof(struct problem_struct));

    // Dynamically allocate memory for problem_id and copy the string
    copy_prob->problem_id = malloc(strlen(prob->problem_id) + 1); // +1 for null terminator
    strcpy(copy_prob->problem_id, prob->problem_id);

    copy_prob->bin_capacity = prob->bin_capacity;
    copy_prob->num_of_items = prob->num_of_items;
    copy_prob->best_obj = prob->best_obj;

    copy_prob->items = malloc(sizeof(struct item_struct) * prob->num_of_items);

    for (int i = 0; i < prob->num_of_items; i++)
    {
        copy_prob->items[i] = prob->items[i];
    }

    return copy_prob;
}

struct bin_struct *copy_bin(struct bin_struct *bin)
{
    struct bin_struct* new_bin = malloc(sizeof(struct bin_struct*));
    new_bin->cap_left = bin->cap_left;
    new_bin->num_of_items_stored = bin->num_of_items_stored;
    new_bin->stored_items = malloc(sizeof(struct item_struct *) * bin->num_of_items_stored);

    for (int i = 0; i < bin->num_of_items_stored; i++)
    {
        new_bin->stored_items[i] = bin->stored_items[i];
    }

    return new_bin;
}

struct problem_struct *problem_delete_item(struct problem_struct *prob, int item_index)
{
    for (int k = item_index; k < prob->num_of_items - 1; k++)
    {
        prob->items[k] = prob->items[k + 1];
    }
    prob->num_of_items--;
    printf("Item %d ,size %d deleted\n", item_index, prob->items[item_index].size);
}

bool compare_bins_content(struct bin_struct *bin1, struct bin_struct *bin2)
{
    printf("Comparing bins content\n");
    if (bin1->num_of_items_stored != bin2->num_of_items_stored)
    {
        return false;
    }
    if (bin1->cap_left != bin2->cap_left)
    {
        return false;
    }
    for (int i = 0; i < bin1->num_of_items_stored; i++)
    {
        if (bin1->stored_items[i]->indx != bin2->stored_items[i]->indx)
        {
            return false;
        }
    }
    return true;
}

struct bin_struct *add_item_to_individual_bin(struct bin_struct *bin, struct item_struct *item)
{
    printf("Num of items stored: %d\n", bin->num_of_items_stored);
    bin->stored_items = realloc(bin->stored_items, (bin->num_of_items_stored + 1) * sizeof(struct item_struct *));
    printf("Checkpoint\n");
    struct item_struct *new_item = malloc(sizeof(struct item_struct));
    printf("1Added item %d:size:%d to individual bin\n", item->indx, item->size);
    new_item->indx = item->indx;
    new_item->size = item->size;

    bin->stored_items[bin->num_of_items_stored] = new_item;
    
    printf("2Added item %d:size:%d to individual bin\n", bin->stored_items[bin->num_of_items_stored]->indx, bin->stored_items[bin->num_of_items_stored]->size);
    bin->cap_left -= new_item->size;
    bin->num_of_items_stored++;
    return bin;
}

struct bin_struct *delete_item_from_individual_bin(struct bin_struct *bin, int item_index)
{
    if (item_index < 0 || item_index >= bin->num_of_items_stored) {
        printf("Invalid item index in delete_item_from_individual_bin\n");
        return NULL;
    }

    int removed_item_size = bin->stored_items[item_index]->size;

    // Shift item's pointers to fill the gap
    for (int i = item_index; i < bin->num_of_items_stored; i++) {
        
        if (i == bin->num_of_items_stored - 1)
        {
            printf("Deleted item %d:size:%d from individual bin\n", bin->stored_items[i]->indx, bin->stored_items[i]->size);
            break;
        }
        bin->stored_items[i] = bin->stored_items[i + 1];
    }

    // Update bin info
    bin->num_of_items_stored--;
    bin->cap_left += removed_item_size;

    return bin;
}

void *rmbs(int gap, int item_index, int chosen_bin_index, struct solution_struct *sln, struct bin_struct *bin, struct problem_struct *prob)
{
    printf("Entered RMBS with item index %d\n", item_index);
    for (int i = item_index; i < prob->num_of_items; i++)
    {
        // printf("Checkpoint 1\n");
        printf("Item size: %d, bin capacity: %d\n", prob->items[i].size, bin->cap_left);
        if (prob->items[i].size <= bin->cap_left)
        {
            // printf("Checkpoint 2\n");
            add_item_to_individual_bin(bin, &prob->items[i]);
            printf("Added item %d of size %d to bin %d\n", prob->items[i].indx, prob->items[i].size, chosen_bin_index);
            
            rmbs(gap, i + 1, chosen_bin_index, sln, bin, prob);

            delete_item_from_individual_bin(bin, bin->num_of_items_stored - 1);
            
            printf("Bins Chosen: %d, %d, cap left: %d\n", chosen_bin_index, sln->bins[chosen_bin_index].num_of_items_stored, sln->bins[chosen_bin_index].cap_left);
            if (sln->bins[chosen_bin_index].cap_left <= gap)
            {
                printf("Leaving RMBS\n");
                return;
            }
        }
        printf("Next item\n");
    }
    for (int i = 0; i < bin->num_of_items_stored; i++)
    {
        printf("Item %d, size %d\n", bin->stored_items[i]->indx, bin->stored_items[i]->size);
    }
    if (bin->cap_left < sln->bins[chosen_bin_index].cap_left)
    {
        //sln->bins[chosen_bin_index] = *bin;
        sln->bins[chosen_bin_index] = *copy_bin(bin);
        sln->bins[chosen_bin_index].cap_left = bin->cap_left;
        sln->bins[chosen_bin_index].num_of_items_stored = bin->num_of_items_stored;
        for (int i = 0; i < bin->num_of_items_stored; i++)
        {
            printf("Item %d, size %d\n", bin->stored_items[i]->indx, bin->stored_items[i]->size);
        }
        printf("Original Bin: %d, cap left: %d\n", bin->num_of_items_stored, bin->cap_left);
        for (int i = 0; i < sln->bins[chosen_bin_index].num_of_items_stored; i++)
        {
            printf("Item %d, size %d\n", sln->bins[chosen_bin_index].stored_items[i]->indx, bin->stored_items[i]->size);
        }
        printf("Chosen Bin[%d]: %d, cap left: %d\n", chosen_bin_index, sln->bins[chosen_bin_index].num_of_items_stored, sln->bins[chosen_bin_index].cap_left);
    }
    // printf("Leaving RMBS\n");
}

void add_bin_at_index(struct solution_struct *sln, int bin_index)
{
    if (bin_index >= sln->objective)
    {   
        printf("Solution has %d bins, adding bin at index %d\n", sln->objective, bin_index);
        sln->bins = realloc(sln->bins, (sln->objective + 1) * sizeof(struct bin_struct));

        if (sln->bins == NULL)
        {
            // Handle error
            printf("Memory allocation when adding bin for solution %s failed!\n", sln->prob->problem_id);
            exit(1); // Exit the program
        }

        sln->bins[bin_index].stored_items = malloc(sizeof(struct item_struct *));
        sln->bins[bin_index].num_of_items_stored = 0;
        sln->bins[bin_index].cap_left = sln->prob->bin_capacity;
    }
}

void *solution_initialisation(struct problem_struct *prob)
{
    struct problem_struct *copy_prob = copy_problem(prob);
    struct solution_struct *instance_solution = init_solution(copy_prob);
    // for (int i = 0; i < instance_solution->prob->num_of_items; i++)
    // {
    //     printf("Copy problem - item : %d %d\n", copy_prob->items[i].indx, copy_prob->items[i].size);
    // }
    // printf("Solution has %d bins\n", instance_solution->objective);
    // for(int i = 0; i < instance_solution->objective; i++)
    // {
    //     printf("Bin %d has %d items\n", i, instance_solution->bins[i].num_of_items_stored);
    // }
    // return 0;

    int gap = 0;
    switch (prob->num_of_items)
    {
    case 120:
        gap = 3;
        break;
    case 500:
        gap = 5;
        break;
    case 200:
        gap = 11000;
        break;
    }

    int chosen_bin_index = 0;
    while (copy_prob->num_of_items != 0)
    {
        printf("Enter while loop\n");
        //if (chosen_bin_index >= instance_solution->objective)
        // add_bin_at_index(instance_solution, chosen_bin_index);
        add_bin(instance_solution);
        printf("Solution has %d bins after adding bin\n", instance_solution->objective);

        add_item_to_bin(instance_solution, chosen_bin_index, &prob->items[0]);
        printf("Item %d size: %d added to bin %d\n", prob->items[0].indx, prob->items[0].size, chosen_bin_index);
        // for (int i = 0; i < instance_solution->bins[chosen_bin_index].num_of_items_stored; i++)
        // {
        //     printf("Bin %d has %d items: %d\n", chosen_bin_index, instance_solution->bins[chosen_bin_index].num_of_items_stored, instance_solution->bins[chosen_bin_index].stored_items[i]->size);
        // }

        // struct bin_struct *temp_bin = &instance_solution->bins[chosen_bin_index];
        struct bin_struct *temp_bin = copy_bin(&instance_solution->bins[chosen_bin_index]);
        // for (int i = 0; i < instance_solution->bins[chosen_bin_index].num_of_items_stored; i++)
        // {
        //     if (instance_solution->bins[chosen_bin_index].stored_items[i]->size == temp_bin->stored_items[i]->size)
        //     {


        //         printf("Same item: %d, temp: %d\n", instance_solution->bins[chosen_bin_index].stored_items[i]->size, temp_bin->stored_items[i]->size);
        //     }
        // }

        printf("Copy problem - item : %d, size: %d\n", copy_prob->items[0].indx, copy_prob->items[0].size);
        problem_delete_item(copy_prob, 0);
        printf("Copy problem - item : %d\n", copy_prob->items[0].indx);

        if (copy_prob->num_of_items == 0)
        {
            printf("Solution has %d bins\n", instance_solution->objective);
            clear_empty_bin(instance_solution);
            printf("Solution has %d bins\n", instance_solution->objective);
            printf("----EXP Checkpoint----\n");
            //return instance_solution;
            return instance_solution;
        }

        rmbs(gap, 0, chosen_bin_index, instance_solution, temp_bin, copy_prob); // For every bin.
        for (int i = 0; i < instance_solution->bins[chosen_bin_index].num_of_items_stored; i++)
        {
            printf("111After RMBS: Bin %d has %d items: Index:%d, SIze:%d\n", chosen_bin_index, instance_solution->bins[chosen_bin_index].num_of_items_stored, instance_solution->bins[chosen_bin_index].stored_items[i]->indx, instance_solution->bins[chosen_bin_index].stored_items[i]->size);
        }

        for (int i = 0; i < instance_solution->bins[chosen_bin_index].num_of_items_stored; i++)
        {
            // delete_item_from_bin(instance_solution, chosen_bin_index, i);
            for (int j = 0; j < copy_prob->num_of_items; j++)
            {
                if (copy_prob->items[j].indx == instance_solution->bins[chosen_bin_index].stored_items[i]->indx)
                {
                    problem_delete_item(copy_prob, j);
                }
            }
        }
        for (int i = 0; i < instance_solution->bins[chosen_bin_index].num_of_items_stored; i++)
        {
            printf("After RMBS: Bin %d has %d items: %d\n", chosen_bin_index, instance_solution->bins[chosen_bin_index].num_of_items_stored, instance_solution->bins[chosen_bin_index].stored_items[i]->size);
        }
        // return 0;
        chosen_bin_index++;
    }
    printf("----Finished Checkpoint----\n");
    printf("Solution has %d bins\n", instance_solution->objective);
    clear_empty_bin(instance_solution);
    printf("Solution has %d bins\n", instance_solution->objective);

    return instance_solution;
}

int main(int argc, const char *argv[])
{
    printf("Starting the run!\n");
    char data_file[50] = {"binpack1.txt"}, out_file[50] = {"test_solution.txt"}, solution_file[50] = {};
    /*if(argc<3)
    {
        printf("Insufficient arguments. Please use the following options:\n   -s data_file (compulsory)\n   -o out_file (default my_solutions.txt)\n   -c solution_file_to_check\n   -t max_time (in sec)\n");
        return 1;
    }
    else if(argc>9)
    {
        printf("Too many arguments.\n");
        return 2;
    }
    else
    {
        for(int i=1; i<argc; i=i+2)
        {
            if(strcmp(argv[i],"-s")==0)
                strcpy(data_file, argv[i+1]);
            else if(strcmp(argv[i],"-o")==0)
                strcpy(out_file, argv[i+1]);
            else if(strcmp(argv[i],"-c")==0)
                strcpy(solution_file, argv[i+1]);
            else if(strcmp(argv[i],"-t")==0)
                MAX_TIME = atoi(argv[i+1]);
        }
        printf("data_file= %s, output_file= %s, sln_file=%s, max_time=%d", data_file, out_file, solution_file, MAX_TIME);
    }*/
    struct problem_struct **my_problems = load_problems(data_file);
    struct solution_struct **my_solutions = (struct solution_struct **)malloc(NUM_OF_PROBLEM * sizeof(struct solution_struct *));

    for (int i = 0; i < NUM_OF_PROBLEM; i++)
    {
        best_sln =  NULL;
        candidate_sln = NULL;

        srand(RAND_SEED[i]);
        // Generate an initial solution and set as best solution
        // my_solutions[i] = first_fit_decreasing(my_problems[i]);
        best_sln = solution_initialisation(my_problems[i]);
        // my_solutions[i] = minimum_bin_slack(my_problems[i]);

        // update_candidate_solution(my_solutions[i]);
        // update_best_solution(my_solutions[i]);
        // SimulatedAnnealing(my_solutions[i]);

        output_solution(best_sln, out_file, i);
    }    
    printf("Checkpoint after output_solution\n");

    for (int i = 0; i < NUM_OF_PROBLEM; i++)
    {
        free_problem(my_problems[i]);
        printf("Memory freed for problem instance %d\n", i);
    }
    for (int i = 0; i < NUM_OF_PROBLEM; i++)
    {
        free_solution(my_solutions[i]);
        printf("Memory freed for solution instance %d\n", i);
    }
    free_solution(best_sln);
    printf("Memory freed for best solution\n");
}

struct solution_struct *best_fit(struct problem_struct *prob)
{
    struct solution_struct *instance_solution = init_solution(prob);

    for (int j = 0; j < prob->num_of_items; j++)
    {
        int best_bin_no = -1;
        int best_capacity = prob->bin_capacity;
        for (int bin_no = 0; bin_no < instance_solution->objective; bin_no++)
        {
            if (instance_solution->bins[bin_no].cap_left >= prob->items[j].size && instance_solution->bins[bin_no].cap_left < best_capacity)
            {
                // printf("Enough space for item %d in bin %d\n", prob->items[j].indx, k);
                best_bin_no = bin_no;
                best_capacity = instance_solution->bins[bin_no].cap_left;
            }
        }
        if (best_bin_no == -1)
        {
            add_bin(instance_solution);
            add_item_to_bin(instance_solution, instance_solution->objective - 1, &prob->items[j]);
        }
        else
        {
            add_item_to_bin(instance_solution, best_bin_no, &prob->items[j]);
        }
    }
    return instance_solution;
} // Function test passed

struct solution_struct *minimum_bin_slack(struct problem_struct *prob)
{
    // Sort item according to size in decreasing order
    qsort(prob->items, prob->num_of_items, sizeof(struct item_struct), cmp_item_size_decreasing);

    return best_fit(prob);
} // Function test passed