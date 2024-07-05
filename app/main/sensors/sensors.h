float temp_tresh_1 = 40.0;
float temp_tresh_2 = 50.0;

float co2_tresh_1 = 15.0;

float fill_tresh_1 = 95.0;

int gathering_times [7];
int gathering_collected = 0;
long long int last_gather = 0;
int avg_windows_to_gather = 0;

float* filling_levels;
int filling_occupation = 0;
int filling_lenght = 0;

float filling_rate = 0.0;

