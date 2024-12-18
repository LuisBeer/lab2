#include "simulation/barnes_hut_simulation_with_collisions.h"
#include "simulation/barnes_hut_simulation.h"
#include "simulation/naive_parallel_simulation.h"
#include <omp.h>

void BarnesHutSimulationWithCollisions::simulate_epochs(Plotter& plotter, Universe& universe, std::uint32_t num_epochs, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    for(int i = 0; i < num_epochs; i++){
        simulate_epoch(plotter, universe, create_intermediate_plots, plot_intermediate_epochs);
    }
}

void BarnesHutSimulationWithCollisions::simulate_epoch(Plotter& plotter, Universe& universe, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    return;
}

void BarnesHutSimulationWithCollisions::find_collisions(Universe& universe){
    // Speichert, ob ein Körper bereits "aufgenommen" wurde
    std::vector is_absorbed(universe.num_bodies, false);

    for(int i = 0; i < universe.num_bodies; i++) {
        if(is_absorbed[i])continue; //überspringe absorbierten Körper

        //finde alle Körper, die mit i Kollidieren
        std::vector collisions = {i};
        int biggest = i; //index des schwersten Körpers
        for(int j = 0; j < universe.num_bodies; j++) {

            if(i == j || is_absorbed[j]) continue; //überspringe absorbierten Körper oder gleichen (i kann nicht mit i kollidieren)

            Vector2d<double> connect = universe.positions[j] - universe.positions[i] ;
            if(connect.norm() < 100000000) {
                collisions.push_back(j);
                if(universe.weights[j] > universe.weights[biggest]) {
                    biggest = j;
                }
            }
        }

        //berechne Gewicht und Geschwindigkeit
        for(int j = 0; j < collisions.size(); j++) {
            if(j == biggest) continue;

            //addiere Gewicht
            double m2 = universe.weights[biggest] + universe.weights[j];

            //Geschwindigkeit nach Impulserhaltung
            universe.velocities[biggest] = (universe.velocities[biggest] * universe.weights[biggest]  + universe.velocities[j] * universe.weights[j]) / m2;

            //neues Gewicht zuweisen
            universe.weights[biggest] = m2;
        }

    }

    //for ()
}

void BarnesHutSimulationWithCollisions::find_collisions_parallel(Universe& universe){
    return;
}