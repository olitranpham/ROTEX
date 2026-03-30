#include <stdio.h>
#include <math.h>

double acromion_accel_x = 1;
double acromion_accel_y = 2;
double acromion_accel_z = 3;
double acromion_torque;
double epicondyle_accel_x = 4;
double epicondyle_accel_y = 5;
double epicondyle_accel_z = 6;
double epicondyle_torque;
float mass = 95;

void main() {
    double net_acromion_accel = sqrt((acromion_accel_x * acromion_accel_x) + (acromion_accel_y * acromion_accel_y)+ (acromion_accel_z * acromion_accel_z));
    printf("Acromion net acceleration: %lf\n", net_acromion_accel);

    double net_epicondyle_accel = sqrt((epicondyle_accel_x * epicondyle_accel_x) + (epicondyle_accel_y * epicondyle_accel_y) + (epicondyle_accel_z * epicondyle_accel_z));
    printf("Epicondyle net acceleration: %lf\n", net_epicondyle_accel);

    double arm_mass = (mass * 0.453592 * 0.0575);
    printf("Arm mass: %lf\n", arm_mass);

    double net_acromion_force = (arm_mass * net_acromion_accel);
    printf("Net acromion force: %lf\n", net_acromion_force);

    double net_epicondyle_force = (arm_mass * net_epicondyle_accel);
    printf("Net epicondyle force: %lf\n", net_epicondyle_force);
}
