#include <stdio.h>
#include <math.h>

double acromion_accel_x = 1;
double acromion_accel_y = 2;
double acromion_accel_z = 3;
double acromion_angular_vel = 7;
double epicondyle_accel_x = 4;
double epicondyle_accel_y = 5;
double epicondyle_accel_z = 6;
double epicondyle_angular_vel = 8;
float mass = 95;
double time_interval = 0.01;

void main() {
    double net_accel = sqrt(((epicondyle_accel_x - acromion_accel_x) * (epicondyle_accel_x - acromion_accel_x)) + ((epicondyle_accel_y - acromion_accel_y) * (epicondyle_accel_y - acromion_accel_y)) + ((epicondyle_accel_z - acromion_accel_z) * (epicondyle_accel_z - acromion_accel_z)));
    printf("Net acceleration: %lf\n", net_accel);

    double arm_mass = (mass * 0.453592 * 0.0575);
    printf("Arm mass: %lf\n", arm_mass);

    double net_force = (arm_mass * net_accel);
    printf("Net force: %lf\n", net_force);

    double angular_vel = (epicondyle_angular_vel * 180 / 3.14159);
    double angle = (angular_vel * time_interval);
    double torque = fabs(net_force * 0.290 * sin(angle));
    printf("Torque: %lf\n", torque);
}
