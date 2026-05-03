#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep()

typedef struct {
    int lane_status;
    int obstacle_distance;
} SensorData_t;

// Event flags (simulate RTOS flags)
int lane_flag = 0;
int obstacle_flag = 0;

void SensorTask(SensorData_t *data)
{
    data->lane_status = rand() % 2;
    data->obstacle_distance = rand() % 100;

    printf("LANE=%d,DIST=%d\n", data->lane_status, data->obstacle_distance);
}

void FilterTask(SensorData_t *data)
{
    data->obstacle_distance += (rand() % 5 - 2);
}

void DecisionTask(SensorData_t *data)
{
    lane_flag = 0;
    obstacle_flag = 0;

    if(data->lane_status == 1)
        lane_flag = 1;

    if(data->obstacle_distance < 20)
        obstacle_flag = 1;
}

void AlertTask()
{
    if(lane_flag)
        printf("ALERT: Lane Deviation!\n");

    if(obstacle_flag)
        printf("ALERT: Obstacle Too Close!\n");
}

int main()
{
    SensorData_t data;

    for(int i = 0; i < 10; i++)
    {
        SensorTask(&data);
        FilterTask(&data);
        DecisionTask(&data);
        AlertTask();

        printf("---------------------\n");

        sleep(1);  // simulate RTOS delay
    }

    return 0;
}