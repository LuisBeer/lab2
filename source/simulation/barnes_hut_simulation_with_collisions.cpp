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
    Quadtree qt = Quadtree(universe, universe.get_bounding_box(), 2);   //construct mode ????

    qt.calculate_center_of_mass();
    qt.calculate_cumulative_masses();

    calculate_forces(universe, qt);

    NaiveParallelSimulation::calculate_velocities(universe);
    NaiveParallelSimulation::calculate_positions(universe);

    find_collisions(universe);

    universe.current_simulation_epoch++;
    if(create_intermediate_plots && (universe.current_simulation_epoch%plot_intermediate_epochs == 0)) {
        plotter.write_and_clear();
    }
}

void BarnesHutSimulationWithCollisions::find_collisions(Universe& universe) {
    std::vector<bool> merged(universe.num_bodies, false); // Trackt, ob ein Körper bereits verschmolzen wurde.

    for (int i = 0; i < universe.num_bodies; ++i) {
        if (merged[i]) continue; // Überspringe bereits verschmolzene Körper.

        for (int j = i + 1; j < universe.num_bodies; ++j) {
            if (merged[j]) continue;

            // Berechne die Distanz zwischen den Körpern.
            double distance = (universe.positions[i] - universe.positions[j]).norm();

            if (distance < 100000000.0) { // Kollision erkannt.
                if (universe.weights[i] >= universe.weights[j]) {
                    // Körper j wird von Körper i aufgenommen.
                    universe.velocities[i] = (universe.velocities[i] * universe.weights[i] + universe.velocities[j] * universe.weights[j]) / (universe.weights[i] + universe.weights[j]);
                    universe.weights[i] += universe.weights[j];
                    universe.weights[j] = 0;
                    merged[j] = true;
                } else {
                    // Körper i wird von Körper j aufgenommen.
                    universe.velocities[j] = (universe.velocities[i] * universe.weights[i]  + universe.velocities[j] * universe.weights[j]) / (universe.weights[i] + universe.weights[j]);
                    universe.weights[j] += universe.weights[i];
                    universe.weights[i] = 0;
                    merged[i] = true;
                    break; // Körper i wurde verschmolzen, keine weiteren Checks nötig.
                }
            }
        }
    }

    // Entferne verschmolzene Körper.
    int new_num_bodies = 0;
    for (int i = 0; i < universe.num_bodies; ++i) {
        if (universe.weights[i] > 0) { // Nur unverschmolzene Körper bleiben erhalten.
            universe.positions[new_num_bodies] = universe.positions[i];
            universe.velocities[new_num_bodies] = universe.velocities[i];
            universe.weights[new_num_bodies] = universe.weights[i];
            ++new_num_bodies;
        }
    }
    universe.num_bodies = new_num_bodies;
}



void BarnesHutSimulationWithCollisions::find_collisions_parallel(Universe& universe) {
    std::vector<bool> merged(universe.num_bodies, false); // Trackt, ob ein Körper bereits verschmolzen wurde.

    #pragma omp parallel
    {
        std::vector<bool> local_merged(universe.num_bodies, false); // Lokale Kopie von merged.

        #pragma omp for schedule(dynamic)
        for (int i = 0; i < universe.num_bodies; ++i) {
            if (local_merged[i]) continue;

            for (int j = i + 1; j < universe.num_bodies; ++j) {
                if (local_merged[j]) continue;

                // Berechne die Distanz zwischen den Körpern.
                double distance = (universe.positions[i] - universe.positions[j]).norm();

                if (distance < 100000000.0) { // Kollision erkannt.
                    if (universe.weights[i] >= universe.weights[j]) {
                        // Körper j wird von Körper i aufgenommen.
                        universe.velocities[i] = (universe.velocities[i] * universe.weights[i] + universe.velocities[j] * universe.weights[j]) / (universe.weights[i] + universe.weights[j]);
                        universe.weights[i] += universe.weights[j];
                        universe.weights[j] = 0;
                        local_merged[j] = true;
                    } else {
                        // Körper i wird von Körper j aufgenommen.
                        universe.velocities[j] = (universe.velocities[i] * universe.weights[i]  + universe.velocities[j] * universe.weights[j]) / (universe.weights[i] + universe.weights[j]);
                        universe.weights[j] += universe.weights[i];
                        universe.weights[i] = 0;
                        local_merged[i] = true;
                        break; // Körper i wurde verschmolzen, keine weiteren Checks nötig.
                    }
                }
            }
        }

        // Synchronisiere Änderungen am `merged` Vektor.
        #pragma omp critical
        for (int i = 0; i < universe.num_bodies; ++i) {
            if (local_merged[i]) {
                merged[i] = true;
            }
        }
    }

    // Entferne verschmolzene Körper.
    int new_num_bodies = 0;
    for (int i = 0; i < universe.num_bodies; ++i) {
        if (universe.weights[i] > 0) { // Nur unverschmolzene Körper bleiben erhalten.
            universe.positions[new_num_bodies] = universe.positions[i];
            universe.velocities[new_num_bodies] = universe.velocities[i];
            universe.weights[new_num_bodies] = universe.weights[i];
            ++new_num_bodies;
        }
    }
    universe.num_bodies = new_num_bodies;
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


/* //Innen Parallel
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
            if(connect.norm() < 100000000000) {
            #pragma omp critical
                {
                is_absorbed[j] = true;
                collisions.push_back(j);
            }
                if(universe.weights[j] > universe.weights[biggest]) {

                    is_absorbed[j] = false;
                    is_absorbed[biggest] = true;
                    biggest = j;
                }
            }
        }


         //prallel variante
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
        */