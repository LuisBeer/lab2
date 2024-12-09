#include "simulation/naive_parallel_simulation.h"
#include "physics/gravitation.h"
#include "physics/mechanics.h"

#include <cmath>

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


void NaiveParallelSimulation::calculate_forces(Universe& universe){
    return;
}

void NaiveParallelSimulation::calculate_velocities(Universe& universe){
    return;
}

void NaiveParallelSimulation::calculate_positions(Universe& universe){
    return;
}