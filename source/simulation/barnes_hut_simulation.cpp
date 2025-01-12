#include "simulation/barnes_hut_simulation.h"
#include "simulation/naive_parallel_simulation.h"
#include "physics/gravitation.h"
#include "physics/mechanics.h"
#include "omp.h"

#include <cmath>

void BarnesHutSimulation::simulate_epochs(Plotter& plotter, Universe& universe, std::uint32_t num_epochs, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    for(int i = 0; i < num_epochs; i++){
        simulate_epoch(plotter, universe, create_intermediate_plots, plot_intermediate_epochs);
    }
}

void BarnesHutSimulation::simulate_epoch(Plotter& plotter, Universe& universe, bool create_intermediate_plots, std::uint32_t plot_intermediate_epochs){
    Quadtree qt = Quadtree(universe, universe.get_bounding_box(), 2);   //construct mode ????

    qt.calculate_center_of_mass();
    qt.calculate_cumulative_masses();

    calculate_forces(universe, qt);

    NaiveParallelSimulation::calculate_velocities(universe);
    NaiveParallelSimulation::calculate_positions(universe);


    universe.current_simulation_epoch++;
    if(create_intermediate_plots && (universe.current_simulation_epoch%plot_intermediate_epochs == 0)) {
        plotter.write_and_clear();
    }

}

void BarnesHutSimulation::get_relevant_nodes(Universe& universe, Quadtree& quadtree, std::vector<QuadtreeNode*>& relevant_nodes, Vector2d<double>& body_position, std::int32_t body_index, double threshold_theta){
    get_relevant_nodes_recursive(quadtree.root, universe, body_position, body_index, threshold_theta, relevant_nodes);
}

void BarnesHutSimulation::get_relevant_nodes_recursive(QuadtreeNode* node, Universe& universe, Vector2d<double>& body_position, std::int32_t body_index, double threshold_theta, std::vector<QuadtreeNode*>& relevant_nodes) {
    //falls null
    if(!node) return;

    // berechne Durchmesser und Abstand kann nicht ins if verlagert werden, da daten schon vor if relevant.
    double d = node->bounding_box.get_diagonal();
    Vector2d<double> bn = (body_position - node->center_of_mass);
    double r = sqrt(std::pow(bn[0], 2) + std::pow(bn[1], 2));

    //wenn threshold erfüllt und K nicht enthalten füge node zu relevant hinzu (Rekursionsanker). Zweite bedingung kann nicht in die If-Verzweigung gelegt werden, da else auch bei nichterfüllen der zweiten bedingung ausgeführt werden muss.
    if(!(node->bounding_box.contains(body_position)) && d / r < threshold_theta) {
        relevant_nodes.push_back(node);
    }
    else {
        // Wenn der Knoten Subquadranten hat, prüfe diese rekursiv
        if(node->body_identifier == -1) {
            for(auto &child : node->children) {
                get_relevant_nodes_recursive(child, universe, body_position, body_index, threshold_theta, relevant_nodes);
            }
        } else if(!node->bounding_box.contains(body_position)){ //wenn body_identifier != -1, dann handelt es sich um einen Blattknoten und er enthält genau einen Himmelskörper. Ein Knoten der also genau einen Himmelskörper enthält aber aufgeteilt werden müsste ist relevant. Darf allerdings nicht K enthalten.
            relevant_nodes.push_back(node);
        }
    }

}


void BarnesHutSimulation::calculate_forces(Universe& universe, Quadtree& quadtree) {
    //gehe alle Körper durch
#pragma omp parallel for default(none) shared(universe, quadtree)
    for(int i = 0; i < universe.num_bodies; i++) {
        auto f = Vector2d<double>(0, 0);
        //berechne alle für Körper relevanten nodes
        auto relevant_nodes = std::vector<QuadtreeNode*>();
        get_relevant_nodes(universe, quadtree, relevant_nodes, universe.positions[i], i, 0.2);

        //gehe durch alle relevanten Nodes und berechne Kraft auf Körper
        for(const QuadtreeNode* node : relevant_nodes) {
            Vector2d<double> bn = (universe.positions[i] - node->center_of_mass);
            double r = sqrt(bn[0] * bn[0] + bn[1] * bn[1]);

            f =  f + bn / r * gravitational_force(universe.weights[i], node->cumulative_mass, r);
        }
        universe.forces[i] = f;
    }
}