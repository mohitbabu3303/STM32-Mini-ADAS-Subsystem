#include <stdio.h>
#include <stdlib.h>

int calculate_alert(int lane, int dist, int speed)
{
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

    fprintf(log, "id,lane,dist,speed,expected,actual,result\n");

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

        int actual = calculate_alert(lane, dist, speed);

        if(actual == expected)
        {
            printf("Test %d: PASS\n", id);
            fprintf(log, "%d,%d,%d,%d,%d,%d,PASS\n",
                    id, lane, dist, speed, expected, actual);
        }
        else
        {
            printf("Test %d: FAIL\n", id);
            fprintf(log, "%d,%d,%d,%d,%d,%d,FAIL\n",
                    id, lane, dist, speed, expected, actual);
        }
    }

    fclose(fp);
    fclose(log);

    printf("\nResults saved in log.csv\n");

    return 0;
}