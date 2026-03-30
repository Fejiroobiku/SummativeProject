#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variable to track operations count
int operation_count = 0;

/**
 * init_array - Performs write operations to allocated memory
 * @arr: Pointer to allocated memory
 * @size: Number of elements in the array
 */
void init_array(int *arr, int size)
{
	int i;
	
	if (arr == NULL)
		return;
	
	for (i = 0; i < size; i++)
	{
		arr[i] = (i + 1) * 5;  // Write to allocated memory
		operation_count++;
	}
}

/**
 * find_max - Finds maximum value in array using conditional branch
 * @arr: Pointer to array
 * @size: Number of elements
 * Return: Maximum value found
 */
int find_max(int *arr, int size)
{
	int i;
	int max_val;
	
	if (arr == NULL || size <= 0)
		return -1;
	
	max_val = arr[0];
	operation_count++;
	
	for (i = 1; i < size; i++)
	{
		if (arr[i] > max_val)  // Conditional branch
			max_val = arr[i];
		operation_count++;
	}
	
	return max_val;
}

/**
 * calc_sum - Calculates sum of array elements
 * @arr: Pointer to array
 * @size: Number of elements
 * Return: Sum of all elements
 */
int calc_sum(int *arr, int size)
{
	int i;
	int sum = 0;
	
	if (arr == NULL)
		return 0;
	
	operation_count++;
	
	for (i = 0; i < size; i++)
	{
		sum += arr[i];
		operation_count++;
	}
	
	return sum;
}

/**
 * main - Entry point for program
 * Return: 0 on success, 1 on failure
 */
int main(void)
{
	int *numbers;
	int size = 6;
	int maximum;
	int total;
	double average;
	
	// Dynamic memory allocation
	numbers = malloc(size * sizeof(int));
	if (numbers == NULL)
		return (1);
	
	// Initialize array with write operations
	init_array(numbers, size);
	
	// Process array data
	maximum = find_max(numbers, size);
	total = calc_sum(numbers, size);
	average = (double)total / size;  // Using standard library implicitly
	
	// Print results to standard output
	printf("=== Array Processing Results ===\n");
	printf("Array initialized with values: ");
	
	// Another loop to display values (not a required function)
	for (int i = 0; i < size; i++)
		printf("%d ", numbers[i]);
	
	printf("\n");
	printf("Maximum value: %d\n", maximum);
	printf("Sum of values: %d\n", total);
	printf("Average value: %.2f\n", average);
	printf("Total operations: %d\n", operation_count);
	
	// Free allocated memory
	free(numbers);
	
	return (0);
}