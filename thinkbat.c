// Nikolai Herrmann 2024

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


const char *START_THRESH = "/sys/class/power_supply/BAT0/charge_control_start_threshold";
const char *STOP_THRESH = "/sys/class/power_supply/BAT0/charge_control_end_threshold";
const char *STAT_SUMMARY = "/sys/class/power_supply/BAT0/uevent";


int write_num_to_file(int num, const char *file_path)
{
    int file_access = access(file_path, W_OK);
    
    if (file_access != 0)
    {
        if (errno == EACCES)
            printf("Permission denied for: %s\n", file_path);
        else
            printf("File access error for: %s\n", file_path);
        return -1;
    }
    
    FILE *file = fopen(file_path, "w");
    
    if (file == NULL)
    {
        printf("Error opening: %s\n", file_path);
        return -1;
    }

    fprintf(file, "%d", num);
    fclose(file);
    
    return 0;
}

int change_thresholds(int start, int stop)
{
    if (start < stop && (start >= 1 && start <= 100) && (stop >= 1 && stop <= 100))
    {
        if (write_num_to_file(start, START_THRESH) == 0)
            return write_num_to_file(stop, STOP_THRESH);
    }
    else
        printf("Start (%d) and/or stop (%d) threshold not within bounds\n", start, stop);

    return -1;
}

char *split(char *str)
{
    while (*str != '\0')
    {
        if (*str == '=')
            break;
        str++;
    }
    return str + 1;
}

void display_summary(char **metrics, int metric_size)
{
    printf("BAT Type:\n");
    printf("\tManufacturer: %s\n", split(metrics[16]));
    printf("\tModel: %s\n", split(metrics[15]));
    printf("\tSerial: %s\n", split(metrics[17]));

    printf("BAT Status:\n");
    float energy_now = atof(split(metrics[12]));
    float energy_full = atof(split(metrics[11]));
    printf("\tPercentage: %.2f\%\n", (energy_now / energy_full) * 100);
    printf("\tState: %s\n", split(metrics[3]));

    float energy_full_design = atof(split(metrics[10]));
    printf("\tCapacity: %.2f\%\n", (energy_full / energy_full_design) * 100);
    printf("\tCycle count: #%s\n", split(metrics[6]));
    float voltage = atof(split(metrics[8])) / 1000000;
    printf("\tVoltage: %.3f V\n", voltage);
}

int read_digit(const char *file_path, int *digit)
{
    FILE *file = fopen(file_path, "r");

    if (file == NULL)
    {
        printf("Error opening: %s", file_path);
        return -1;
    }

    if (fscanf(file, "%d", digit) == EOF)
    {
        printf("Failed to read digit");
        return -1;
    }

    fclose(file);

    return 0;
}

int display_thresh()
{
    int start;
    int stop;
    if (read_digit(START_THRESH, &start) == 0 && read_digit(STOP_THRESH, &stop) == 0)
    {
        printf("\tStart thresh: %d\%\n", start);
        printf("\tStop thresh: %d\%\n", stop);
        return 0;
    }
    return -1;    
}

void *check_resize(int size, int *max_size, int type_size, void *arr)
{
    if (size > *max_size)
    {
        *max_size *= 2;
        arr = realloc(arr, type_size * *max_size);
    }
    return arr;
}

int stat_summary(const char *file_path)
{
    FILE *file = fopen(file_path, "r");
    
    if (file == NULL)
    {
        printf("Error opening: %s", file_path);
        return -1;
    }
    
    int metric_mem_size = 5;
    int metric_idx = 0;
    char **metrics = malloc(metric_mem_size * sizeof(char *));

    int str_mem_size = 5;
    int str_idx = 0;
    char *str = malloc(str_mem_size * sizeof(char));

    do
    {
        char ch = fgetc(file);
        if (ch == EOF)
            break;
        if (ch == '\n')
        {
            str[str_idx] = '\0';
            metrics = check_resize(metric_idx + 1, &metric_mem_size, sizeof(char *), metrics);
            metrics[metric_idx++] = str;
            str = malloc(str_mem_size * sizeof(char));
            str_idx = 0;
        }
        else 
        {
            str = check_resize(str_idx + 2, &str_mem_size, sizeof(char), str);
            str[str_idx++] = ch;
        }
    } 
    while (1);

    display_summary(metrics, metric_idx);
    display_thresh();

    free(str);
    for (int i = 0; i < metric_idx; i++)
        free(metrics[i]);
    free(metrics);
    fclose(file);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc >= 3)
    {
        int start = atoi(argv[1]);
        int stop = atoi(argv[2]);
        int change_status = change_thresholds(start, stop);
        printf((change_status == 0) ? "Successfully changed thresholds\n" : "Failed to change thresholds\n");
    }
    else if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
        printf("Usage:  thinkbat [start charging percentage] [stop charging percentage]\n\tthinkbat 75 80\n");
    else
        stat_summary(STAT_SUMMARY);

    return 0;
}