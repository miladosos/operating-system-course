#include <stdio.h>
#include <math.h>
#include <pthread.h>

int numbers[100], numbers_count;

double numbers_average, numbers_minimum, numbers_maximum, numbers_sd;

void *average() {
    int i;
    numbers_average = 0;
    for (i = 0; i < numbers_count; i++) {
        numbers_average += numbers[i];
    }
    numbers_average /= numbers_count;

    return NULL;
}

void *maximum() {
    numbers_maximum = numbers[0];

    int i;
    for (i = 1; i < numbers_count; i++) {
        if (numbers[i] > numbers_maximum) {
            numbers_maximum = numbers[i];
        }
    }

    return NULL;
}

void *minimum() {
    numbers_minimum = numbers[0];

    int i;
    for (i = 1; i < numbers_count; i++) {
        if (numbers[i] < numbers_minimum) {
            numbers_minimum = numbers[i];
        }
    }

    return NULL;
}

void *standard_deviation() {
    double sum = 0.0, average, sd = 0.0;
    int i;
    for (i = 0; i < numbers_count; ++i) {
        sum += numbers[i];
    }

    average = sum / numbers_count;
    for (i = 0; i < numbers_count; ++i)
        sd += pow(numbers[i] - average, 2);

    numbers_sd = sqrt(sd / numbers_count);
}

int main() {

    printf("how many numbers you're going to input? ");
    scanf("%d", &numbers_count);

    int i;
    for (i = 0; i < numbers_count; i++) {
        scanf("%d", &numbers[i]);
    }

    pthread_t thread_average;
    pthread_t thread_maximum;
    pthread_t thread_minimum;
    pthread_t thread_sd;

    pthread_create(&thread_average, NULL, &average, NULL);
    pthread_create(&thread_maximum, NULL, &maximum, NULL);
    pthread_create(&thread_minimum, NULL, &minimum, NULL);
    pthread_create(&thread_sd, NULL, &standard_deviation, NULL);

    pthread_join(thread_average, NULL);
    pthread_join(thread_maximum, NULL);
    pthread_join(thread_minimum, NULL);
    pthread_join(thread_sd, NULL);

    printf("Average: %f\n", numbers_average);
    printf("Maximum: %f\n", numbers_maximum);
    printf("Minimum: %f\n", numbers_minimum);
    printf("Standard Deviation: %f\n", numbers_sd);

    return 0;
}