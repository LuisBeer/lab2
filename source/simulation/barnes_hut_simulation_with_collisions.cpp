#include "simulation/barnes_hut_simulation_with_collisions.h"
//#include "simulation/barnes_hut_simulation.h"
#include "simulation/naive_parallel_simulation.h"
//#include <omp.h>

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
            if(collisions[j] == biggest) continue; //collisions[j] anstatt j selbst, j der index von colisions ist, biggest jedoch ein index im universe. colisions[j] dagegen speichert die Indizes des Universe.
            //addiere Gewicht
            double m2 = universe.weights[biggest] + universe.weights[j];

            //Geschwindigkeit nach Impulserhaltung
            universe.velocities[biggest] = (universe.velocities[biggest] * universe.weights[biggest]  + universe.velocities[j] * universe.weights[j]) / m2;

            //neues Gewicht zuweisen
            universe.weights[biggest] = m2;
        }
    }

    //update Universe
    std::vector<double> remaining_weights;
    std::vector<Vector2d<double>> remaining_positions;
    std::vector<Vector2d<double>> remaining_velocities;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            remaining_positions.push_back(universe.positions[i]);
            remaining_velocities.push_back(universe.velocities[i]);
            remaining_weights.push_back(universe.weights[i]);
        }
    }

    universe.velocities = remaining_velocities;
    universe.weights = remaining_weights;
    universe.positions = remaining_positions;

}

void BarnesHutSimulationWithCollisions::find_collisions_parallel(Universe& universe){
    // Speichert, ob ein Körper bereits "aufgenommen" wurde
    std::vector is_absorbed(universe.num_bodies, false);
#pragma omp parallel for
    for(int i = 0; i < universe.num_bodies; i++) {
        if(is_absorbed[i])continue; //überspringe absorbierten Körper ---------- ACHTUNG !!!    Momentan race condition. is_absorbed wird evtl. noch in einem Thread auf true gesetzt hat heir aber noch den wert false.
                                    //in #pragma op critical wird noch mal geprüft, deswegen entsteht kein falsches ergebnis, jedoch leidet die effizienz, am besten noch mal optimieren

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

        #pragma omp critical
        {
            //berechne Gewicht und Geschwindigkeit
            for(int j = 0; j < collisions.size(); j++) {
                if(collisions[j] == biggest) continue; //collisions[j] anstatt j selbst, j der index von collisions ist, biggest jedoch ein index im universe. collisions[j] dagegen speichert die Indizes des Universe.


                if(is_absorbed[collisions[j]]) continue; //erneute Prüfung, falls oben fehler durch race condition
                //addiere Gewicht
                double m2 = universe.weights[biggest] + universe.weights[j];

                //Geschwindigkeit nach Impulserhaltung
                universe.velocities[biggest] = (universe.velocities[biggest] * universe.weights[biggest]  + universe.velocities[j] * universe.weights[j]) / m2;

                //neues Gewicht zuweisen
                universe.weights[biggest] = m2;

                //Markiere absorbierte Körper
                is_absorbed[collisions[j]] = true;;
            }
        }
    }

    //update Universe
    std::vector<double> remaining_weights;
    std::vector<Vector2d<double>> remaining_positions;
    std::vector<Vector2d<double>> remaining_velocities;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            remaining_positions.push_back(universe.positions[i]);
            remaining_velocities.push_back(universe.velocities[i]);
            remaining_weights.push_back(universe.weights[i]);
        }
    }

    universe.velocities = remaining_velocities;
    universe.weights = remaining_weights;
    universe.positions = remaining_positions;
}