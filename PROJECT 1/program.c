/**
 * Project 1: ELF Binary Investigation
 *
 * A C program demonstrating:
 * - 3 user-defined functions + main()
 * - Global variable usage
 * - Loop and decision-making statements
 * - Dynamic memory allocation (malloc)
 * - C standard library function call (printf, rand, time)
 * - Meaningful terminal output
 *
 * Compilation: gcc -Wall -O0 -fno-inline -o program program.c
 * Stripping:   strip program
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* ===== Global Variable ===== */
int g_total_numbers = 0; // Tracks how many numbers were processed globally

/* ===== User-defined Function 1: Generate dynamic data ===== */
int *generate_data(int count)
{
    int *data = (int *)malloc(count * sizeof(int));
    if (data == NULL)
    {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }

    /* Loop: Fill array with random values */
    for (int i = 0; i < count; i++)
    {
        data[i] = rand() % 1000; // Random numbers between 0-999
    }

    g_total_numbers = count;
    return data;
}

/* ===== User-defined Function 2: Analyze data (min, max, sum) ===== */
void analyze_data(int *data, int count, int *min, int *max, int *sum)
{
    if (data == NULL || count <= 0)
    {
        fprintf(stderr, "Invalid data for analysis.\n");
        return;
    }

    *min = data[0];
    *max = data[0];
    *sum = 0;

    for (int i = 0; i < count; i++)
    {
        /* Decision-making: Update min */
        if (data[i] < *min)
        {
            *min = data[i];
        }
        /* Decision-making: Update max */
        if (data[i] > *max)
        {
            *max = data[i];
        }
        *sum += data[i];
    }
}

/* ===== User-defined Function 3: Display formatted report ===== */
void display_report(int *data, int count, int min, int max, int sum)
{
    /* C standard library function: printf for formatted output */
    printf("\n========================================\n");
    printf("      DATA ANALYSIS REPORT\n");
    printf("========================================\n");
    printf("Total numbers processed (global): %d\n", g_total_numbers);
    printf("Array size analyzed:              %d\n", count);
    printf("----------------------------------------\n");
    printf("Generated Data: [");

    /* Loop: Print all data values */
    for (int i = 0; i < count; i++)
    {
        if (i < count - 1)
        {
            printf("%3d, ", data[i]);
        }
        else
        {
            printf("%3d", data[i]);
        }
    }
    printf("]\n");
    printf("----------------------------------------\n");
    printf("Minimum value:  %d\n", min);
    printf("Maximum value:  %d\n", max);
    printf("Sum of values:  %d\n", sum);
    printf("Average value:  %.2f\n", (double)sum / count);
    printf("========================================\n");
}

/* ===== main(): Program entry point ===== */
int main()
{
    int count;
    int min, max, sum;

    printf("ELF Binary Investigation - Project 1\n");
    printf("====================================\n");

    /* Seed the random number generator using time library function */
    srand((unsigned int)time(NULL));

    /* Ask user for the number of data points */
    printf("Enter the number of data points to generate: ");
    scanf("%d", &count);

    /* Decision-making: Validate input */
    if (count <= 0)
    {
        printf("Invalid input. Using default value of 10.\n");
        count = 10;
    }

    /* Dynamically allocate and generate data */
    int *dataset = generate_data(count);

    /* Analyze the generated data */
    analyze_data(dataset, count, &min, &max, &sum);

    /* Display the analysis report */
    display_report(dataset, count, min, max, sum);

    /* Free dynamically allocated memory */
    free(dataset);
    printf("Memory successfully freed. Program terminating.\n");

    return 0;
}
