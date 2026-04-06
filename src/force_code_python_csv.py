import time
import math
import pandas       #note you will need to install pandas. Use command "pip install pandas" in terminal

'''
acromion_accel_x = 1
acromion_accel_y = 2
acromion_accel_z = 3
acromion_angular_vel = 7
epicondyle_accel_x = 4
epicondyle_accel_y = 5
epicondyle_accel_z = 6
epicondyle_angular_vel = 8
'''

#User inputted variables
max_torque = 0.2        #actual value 46.1
mass = 95
time_interval = 0.01    #remove and calculate time interval instead

#Loops - do not touch
iteration = 2
flag = 0
max_torque_flag = 0

try:
    data = pandas.read_csv("src/sample_IMU_data.csv")

    while (flag == 0):
        time_collected = data.iat[1, iteration]
        acromion_accel_x = data.iat[2, iteration]
        acromion_accel_y = data.iat[3, iteration]
        acromion_accel_z = data.iat[4, iteration]
        acromion_angular_vel = data.iat[5, iteration]
        epicondyle_accel_x = data.iat[6, iteration]
        epicondyle_accel_y = data.iat[7, iteration]
        epicondyle_accel_z = data.iat[8, iteration]
        epicondyle_angular_vel = data.iat[9, iteration]

        net_accel = math.sqrt(((epicondyle_accel_x - acromion_accel_x) ** 2) + ((epicondyle_accel_y - acromion_accel_y) ** 2) + ((epicondyle_accel_z - acromion_accel_z) ** 2))
        print(f"Net acceleration: {net_accel}")

        arm_mass = (mass * 0.453592 * 0.0575)
        print(f"Arm mass: {arm_mass}")

        net_force = (arm_mass * net_accel)
        print(f"Net force: {net_force}")

        angular_vel = (epicondyle_angular_vel * 180 / 3.14159)
        angle = (angular_vel * time_interval)
        torque = abs(net_force * 0.290 * math.sin(angle))
        print(f"Torque: {torque}")
        
        print(" ")

        if torque > max_torque:
            max_torque_flag = max_torque_flag + 1
            print("Error: max torque exceeded.")
            print(" ")

        iteration = iteration + 1

        time.sleep(1)

except KeyboardInterrupt:
    print("Error. Testing concluded.")

finally:
    print(f"Total times exceeding threshold: {max_torque_flag}")
    print("Calculations complete. Testing concluded.")