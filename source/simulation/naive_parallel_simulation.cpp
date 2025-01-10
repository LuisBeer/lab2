#include "simulation/naive_parallel_simulation.h"
#include "physics/gravitation.h"
#include "physics/mechanics.h"

#include <cmath>
//hab ich hinzugefügt
#include "constants.h"
#include <iostream>

void NaiveParallelSimulation::simulate_epochs(Plotter& plotter, Universe& universe, std::uint32_t num_epochs, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    for(int i = 0; i < num_epochs; i++){
        simulate_epoch(plotter, universe, create_intermediate_plots, plot_intermediate_epochs);
    }
}

void NaiveParallelSimulation::simulate_epoch(Plotter& plotter, Universe& universe, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    calculate_forces(universe);
    calculate_velocities(universe);
    calculate_positions(universe);
    universe.current_simulation_epoch++;
    if(create_intermediate_plots){
        if(universe.current_simulation_epoch % plot_intermediate_epochs == 0){
            plotter.add_bodies_to_image(universe);
            plotter.write_and_clear();
        }
    }
}


/**void NaiveParallelSimulation::calculate_forces(Universe &universe) {
    #pragma omp parallel for
    for (int i = 0; i < universe.num_bodies; i++) {
        Vector2d<double> f(0.0, 0.0);

        for (int j = 0; j < universe.num_bodies; j++) {
            if (i == j) continue; //Eigeneabhängigkeit ist unnötig

            //Verbindungsvektor
            Vector2d<double> connect = universe.positions[j] - universe.positions[i];
            double d = connect.norm();

            f = f + connect / d * gravitational_force(universe.weights[i], universe.weights[j], d);
        }
        universe.forces[i] = f;
    }
}**/

void NaiveParallelSimulation::calculate_forces(Universe &universe) {
    for (int i = 0; i < universe.num_bodies; i++) {
        Vector2d<double> f(0.0, 0.0);

        for (int j = 0; j < universe.num_bodies; j++) {
            if (i == j) continue; // Ignoriere die Eigeneinwirkung

            // Verbindungsvektor berechnen
            Vector2d<double> connect = universe.positions[j] - universe.positions[i];
            double d = connect.norm();

            // Gravitationkraft berechnen und aufsummieren
            f = f + connect / d * gravitational_force(universe.weights[i], universe.weights[j], d);
        }

        // Berechnete Kraft speichern
        universe.forces[i] = f;
    }
}


/**void NaiveParallelSimulation::calculate_velocities(Universe &universe) {
    //calculate_forces(universe);

    //Parallele Schleife
    #pragma omp parallel for
    for (int i = 0; i < universe.num_bodies; i++) {

      	//Debugging
      	if (universe.forces[i][0] == 0 && universe.forces[i][1] == 0) {
            // Keine Kraft -> Geschwindigkeit bleibt unverändert
            continue;
        }

        //Beschleunigung
        Vector2d<double> a = calculate_acceleration(universe.forces[i], universe.weights[i]);
        //Geschwindigkeit
        Vector2d<double> new_velocity = calculate_velocity(a, universe.velocities[i], epoch_in_seconds);
        universe.velocities[i] = new_velocity;
    }
}**/
void NaiveParallelSimulation::calculate_velocities(Universe &universe) {
    for (int i = 0; i < universe.num_bodies; i++) {
        Vector2d<double> a = calculate_acceleration(universe.forces[i], universe.weights[i]);
        universe.velocities[i] = calculate_velocity(a, universe.velocities[i], epoch_in_seconds);
    }
}

/**void NaiveParallelSimulation::calculate_positions(Universe &universe) {
    calculate_velocities(universe);

    #pragma omp parallel for
    for (int i = 0; i < universe.num_bodies; i++) {
        Vector2d<double> ds = universe.velocities[i] * epoch_in_seconds;

        universe.positions[i] = universe.positions[i] + ds;
    }
}**/

void NaiveParallelSimulation::calculate_positions(Universe &universe) {
    for (int i = 0; i < universe.num_bodies; i++) {
        // Positionsänderung berechnen
        Vector2d<double> ds = universe.velocities[i] * epoch_in_seconds;

        // Neue Position berechnen und speichern
        universe.positions[i] = universe.positions[i] + ds;
    }
}