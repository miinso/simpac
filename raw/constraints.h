#include "types.h"
#include <flecs.h>


struct Position_Constraint_Preprocessed_Data {
    flecs::entity e1;
    flecs::entity e2;
    Vector3r r1_wc; // world quantity
    Vector3r r2_wc;
    Matrix3r e1_world_inverse_inertia_tensor;
    Matrix3r e2_world_inverse_inertia_tensor;
};

struct Angular_Constraint_Preprocessed_Data {
    flecs::entity e1;
    flecs::entity e2;
    Matrix3r e1_world_inverse_inertia_tensor;
    Matrix3r e2_world_inverse_inertia_tensor;
};


// Positional Constraint
void calculate_positional_constraint_preprocessed_data (flecs::entity e1,
flecs::entity e2,
Vector3r r1_lc,
Vector3r r2_lc,
Position_Constraint_Preprocessed_Data& pcpd);

Real positional_constraint_get_delta_lambda (Position_Constraint_Preprocessed_Data& pcpd,
Real h,
Real compliance,
Real lambda,
Vector3r delta_x,
Real violation);

void positional_constraint_apply (Position_Constraint_Preprocessed_Data& pcpd,
Real delta_lambda,
Vector3r delta_x);

// Angular Constraint
void calculate_angular_constraint_preprocessed_data (flecs::entity e1,
flecs::entity e2,
Angular_Constraint_Preprocessed_Data& acpd);

Real angular_constraint_get_delta_lambda (Angular_Constraint_Preprocessed_Data& acpd,
Real h,
Real compliance,
Real lambda,
Vector3r delta_q);

void angular_constraint_apply (Angular_Constraint_Preprocessed_Data& acpd,
Real delta_lambda,
Vector3r delta_q);
