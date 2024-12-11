#include "simulation/barnes_hut_simulation.h"
#include "simulation/naive_parallel_simulation.h"
#include "physics/gravitation.h"
#include "physics/mechanics.h"

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
    return;
}

void BarnesHutSimulation::calculate_forces(Universe& universe, Quadtree& quadtree){
    return;
}