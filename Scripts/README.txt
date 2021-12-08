Script based on https://electromake.pl/how-to-remove-noise-from-accelerometer-data/

To run the script with the gyroscope data, change the line 11 of the script to: data = csvread("gyro_data.txt")

The script is kinda messy but it's only being used to obtain the FIR coeficients used by the mpu6050.c functions.