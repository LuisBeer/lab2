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
            if(connect.norm() < 100000000000) {
                is_absorbed[j] = true; // erstmal auf absorbiert setzen
                //std::cout << i << " " << connect.norm() << std::endl;
                collisions.push_back(j);
                if(universe.weights[j] > universe.weights[biggest]) { //Vergleiche und aktualisiere schwersten Körper
                    is_absorbed[j] = false;
                    is_absorbed[biggest] = true; //neuer schwerster himmelskörper j wird nicht mehr absorbiert aber der alte nun nicht mehr schwerste wird absorbiert
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
    std::vector<Vector2d<double>> remaining_forces;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            remaining_positions.push_back(universe.positions[i]);
            remaining_velocities.push_back(universe.velocities[i]);
            remaining_weights.push_back(universe.weights[i]);
            remaining_forces.push_back(universe.forces[i]);
        }
    }

    universe.num_bodies = remaining_positions.size();
    universe.velocities = remaining_velocities;
    universe.weights = remaining_weights;
    universe.positions = remaining_positions;
    universe.forces = remaining_forces;

    universe.velocities.resize(universe.num_bodies);
    universe.weights.resize(universe.num_bodies);
    universe.positions.resize(universe.num_bodies);
    universe.velocities.resize(universe.num_bodies);
}

void BarnesHutSimulationWithCollisions::find_collisions_parallel(Universe& universe){
    // Speichert, ob ein Körper bereits "aufgenommen" wurde
    std::vector is_absorbed(universe.num_bodies, false);

    for(int i = 0; i < universe.num_bodies; i++) {
        if(is_absorbed[i])continue;

        //finde alle Körper, die mit i Kollidieren
        std::vector collisions = {i};
        int biggest = i; //index des schwersten Körpers
        #pragma omp parallel for default(none) shared(universe, i, is_absorbed, collisions, biggest)
        for(int j = 0; j < universe.num_bodies; j++) {

            if(i == j || is_absorbed[j]) continue; //überspringe absorbierten Körper oder gleichen (i kann nicht mit i kollidieren)

            Vector2d<double> connect = universe.positions[j] - universe.positions[i] ;
            if(connect.norm() < 100000000) {
                #pragma omp critical
                collisions.push_back(j);
                if(universe.weights[j] > universe.weights[biggest]) {
                    #pragma omp critical
                    biggest = j;
                }
            }
        }

        //berechne Gewicht und Geschwindigkeit
        double vmGes_x = 0;
        double vmGes_y = 0;
        double mGes = 0;

        #pragma omp parallel for reduction(+:mGes, vmGes_x, vmGes_y)
        for(int j = 0; j < collisions.size(); j++) {
            //if(is_absorbed[collisions[j]]) continue; //erneute Prüfung, falls oben fehler durch race condition// kann weg glaub ich weil nicht race condition weg
            //addiere Gewicht
            vmGes_x += universe.velocities[j][0] * universe.weights[j];
            vmGes_y += universe.velocities[j][1] * universe.weights[j];
            mGes += universe.weights[j];
            //Markiere absorbierte Körper
            if(collisions[j] != biggest) //collisions[j] anstatt j selbst, j der index von collisions ist, biggest jedoch ein index im universe. collisions[j] dagegen speichert die Indizes des Universe.
                is_absorbed[collisions[j]] = true;
        }
        universe.weights[biggest] = mGes;
        universe.velocities[biggest] = Vector2d<double>(vmGes_x, vmGes_y) / mGes;
    }

    //update Universe
    std::vector<double> remaining_weights;
    std::vector<Vector2d<double>> remaining_positions;
    std::vector<Vector2d<double>> remaining_velocities;
    std::vector<Vector2d<double>> remaining_forces;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            remaining_positions.push_back(universe.positions[i]);
            remaining_velocities.push_back(universe.velocities[i]);
            remaining_weights.push_back(universe.weights[i]);
            remaining_forces.push_back(universe.forces[i]);
        }
    }

    universe.num_bodies = remaining_positions.size();
    universe.velocities = remaining_velocities;
    universe.weights = remaining_weights;
    universe.positions = remaining_positions;
    universe.forces = remaining_forces;

    universe.velocities.resize(universe.num_bodies);
    universe.weights.resize(universe.num_bodies);
    universe.positions.resize(universe.num_bodies);
    universe.velocities.resize(universe.num_bodies);
}



/* //prallelisierung der äußeren schleife
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
*/