#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int calculate_alert(int lane, int dist, int speed)
{
    // Force real computation
    volatile int dummy = 0;
    for(int i = 0; i < 1000000; i++)
    {
        dummy += i;
    }

    if(speed < 30)
        return 0;

    int threshold;

    if(speed <= 30) threshold = 10;
    else if(speed <= 50) threshold = 20;
    else if(speed <= 80) threshold = 30;
    else if(speed <= 100) threshold = 40;
    else threshold = 50;

    if(lane == 1 || dist < threshold)
        return 1;

    return 0;
}

int main()
{
    FILE *fp = fopen("test_cases.csv", "r");
    FILE *log = fopen("log.csv", "w");

    if(fp == NULL)
    {
        printf("Error opening test file\n");
        return 1;
    }

    char line[100];

    // Skip header
    fgets(line, sizeof(line), fp);

   fprintf(log, "id,lane,dist,speed,expected,actual,result,time\n");

    while(fgets(line, sizeof(line), fp))
    {
        int id, lane, dist, speed, expected;

        sscanf(line, "%d,%d,%d,%d,%d",
               &id, &lane, &dist, &speed, &expected);

        // Error handling
        if(dist < 0 || dist > 100)
        {
            printf("Test %d: Invalid distance\n", id);
            continue;
        }

        clock_t start = clock();

int actual = calculate_alert(lane, dist, speed);

clock_t end = clock();

double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

        if(actual == expected)
        {
            printf("Test %d: PASS\n", id);
            fprintf(log,  "%d,%d,%d,%d,%d,%d,PASS,%f\n", 
                           id, lane, dist, speed, expected, actual, time_taken);
        }
        else
        {
            printf("Test %d: FAIL\n", id);
            fprintf(log, "%d,%d,%d,%d,%d,%d,FAIL,%f\n", 
                         id, lane, dist, speed, expected, actual, time_taken);
        }
    }

    fclose(fp);
    fclose(log);

    printf("\nResults saved in log.csv\n");

    return 0;
}