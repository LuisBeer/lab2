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

void BarnesHutSimulationWithCollisions::find_collisions(Universe& universe){
    std::vector<int> sorted_indices;
    for (int i = 0; i < universe.num_bodies; i++) {
        sorted_indices.push_back(i);
    }
    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](int a, int b) {
        return universe.weights[a] > universe.weights[b];
    });

    for (int i = 0; i < universe.num_bodies; i++) {
        std::cout << sorted_indices[i] << " | "<< universe.weights[sorted_indices[i]] << std::endl;
    }
    // Speichert, ob ein Körper bereits "aufgenommen" wurde
    std::vector is_absorbed(universe.num_bodies, false);

    for(int i = 0; i < sorted_indices.size(); i++) {
        if(is_absorbed[sorted_indices[i]])continue; //überspringe absorbierten Körper
        //finde alle Körper, die mit i Kollidieren

        for(int j = i+1; j < sorted_indices.size(); j++) {
            if(i == j || is_absorbed[sorted_indices[j]]) continue; //überspringe absorbierten Körper oder gleichen (i kann nicht mit i kollidieren)

            Vector2d<double> connect = universe.positions[sorted_indices[j]] - universe.positions[sorted_indices[i]] ;
            if(connect.norm() < 100000000000) {
                is_absorbed[sorted_indices[j]] = true;
                double m2 = universe.weights[sorted_indices[i]] + universe.weights[sorted_indices[j]];

                //Geschwindigkeit nach Impulserhaltung
                universe.velocities[sorted_indices[i]] = (universe.velocities[sorted_indices[i]] * universe.weights[sorted_indices[i]]  + universe.velocities[sorted_indices[j]] * universe.weights[sorted_indices[j]]) / m2;

                //neues Gewicht zuweisen
                universe.weights[sorted_indices[i]] = m2;
            }
        }
    }
   /*
    //remove collided objects from universe
    size_t num_bodies = universe.num_bodies;
    std::vector<double> new_weights(num_bodies);
    std::vector<Vector2d<double>> new_positions(num_bodies);
    std::vector<Vector2d<double>> new_velocities(num_bodies);
    std::vector<Vector2d<double>> new_forces(num_bodies);
    for (size_t i = 0; i < sorted_indices.size(); i++)
    {
        if (!is_absorbed[i])
        {
            size_t index = sorted_indices[i];
            new_weights[i] = universe.weights[index];
            new_positions[i] = universe.positions[index];
            new_velocities[i] = universe.velocities[index];
            new_forces[i] = universe.forces[index];
        }
    }
    new_weights.shrink_to_fit();
    universe.weights = new_weights;
    new_positions.shrink_to_fit();
    universe.positions = new_positions;
    new_velocities.shrink_to_fit();
    universe.velocities = new_velocities;
    new_forces.shrink_to_fit();
    universe.forces = new_forces;
    universe.num_bodies = new_weights.size();
    */

    //update Universe
    std::vector<double> remaining_weights;
    std::vector<Vector2d<double>> remaining_positions;
    std::vector<Vector2d<double>> remaining_velocities;
    std::vector<Vector2d<double>> remaining_forces;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            size_t index = sorted_indices[i];
            remaining_positions.push_back(universe.positions[index]);
            remaining_velocities.push_back(universe.velocities[index]);
            remaining_weights.push_back(universe.weights[index]);
            remaining_forces.push_back(universe.forces[index]);
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

void BarnesHutSimulationWithCollisions::find_collisions_parallel(Universe& universe) {
    std::vector<int> sorted_indices;
    for (int i = 0; i < universe.num_bodies; i++) {
        sorted_indices.push_back(i);
    }
    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](int a, int b) {
        return universe.weights[a] > universe.weights[b];
    });

    // Speichert, ob ein Körper bereits "aufgenommen" wurde
    std::vector is_absorbed(universe.num_bodies, false);


    for(int i = 0; i < sorted_indices.size(); i++) {
        if(is_absorbed[sorted_indices[i]])continue; //überspringe absorbierten Körper
        //finde alle Körper, die mit i Kollidieren

#pragma omp parallel for
        for(int j = i+1; j < sorted_indices.size(); j++) {
            if(i == j || is_absorbed[sorted_indices[j]]) continue; //überspringe absorbierten Körper oder gleichen (i kann nicht mit i kollidieren)

            Vector2d<double> connect = universe.positions[sorted_indices[j]] - universe.positions[sorted_indices[i]] ;
            if(connect.norm() < 100000000000) {
                is_absorbed[sorted_indices[j]] = true;
                double m2 = universe.weights[sorted_indices[i]] + universe.weights[sorted_indices[j]];

                //Geschwindigkeit nach Impulserhaltung
                universe.velocities[sorted_indices[i]] = ((universe.velocities[sorted_indices[i]] * universe.weights[sorted_indices[i]])  + (universe.velocities[sorted_indices[j]] * universe.weights[sorted_indices[j]])) / m2;

                //neues Gewicht zuweisen
                universe.weights[sorted_indices[i]] = m2;
            }
        }
    }

    //update Universe
    std::vector<double> remaining_weights;
    std::vector<Vector2d<double>> remaining_positions;
    std::vector<Vector2d<double>> remaining_velocities;
    std::vector<Vector2d<double>> remaining_forces;

    for (int i = 0; i < universe.num_bodies; i++) {
        if(!is_absorbed[i]) {
            size_t index = sorted_indices[i];
            remaining_positions.push_back(universe.positions[index]);
            remaining_velocities.push_back(universe.velocities[index]);
            remaining_weights.push_back(universe.weights[index]);
            remaining_forces.push_back(universe.forces[index]);
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