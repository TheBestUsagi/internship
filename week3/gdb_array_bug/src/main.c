#include <stdio.h>
#include <stdlib.h>

void print_array(int *arr, int size) {
    for (int i = 0; i < size; i++) printf("%d ", arr[i]);
    printf("\n");
}

int main() {
    int size = 5;
    int *array = (int*)malloc(size * sizeof(int));
    if (!array) { puts("Memory allocation failed"); return 1; }

    for (int i = 0; i < size; i++) array[i] = i + 1;

    printf("Original array: ");
    print_array(array, size);

    // 修复：只修改最后一个有效元素（索引 4），不越界
    array[size - 1] = 10;

    printf("Modified array: ");
    print_array(array, size);

    free(array);
    return 0;
}
