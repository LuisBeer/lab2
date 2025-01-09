#include "quadtreeNode.h"

#include <iostream>


double QuadtreeNode::calculate_node_cumulative_mass(){
    if (children.empty()) { // Blattknoten
        cumulative_mass_ready = true;
        return cumulative_mass; // Rückgabe der gespeicherten Masse
    }

    cumulative_mass = 0.0; // Reset
    for (auto* child : children) {
        cumulative_mass += child->calculate_node_cumulative_mass(); // Rekursiver Aufruf
    }
    cumulative_mass_ready = true;
    return cumulative_mass;
}

QuadtreeNode::QuadtreeNode(BoundingBox arg_bounding_box)
        : bounding_box(arg_bounding_box),
		body_identifier(-1), cumulative_mass(0.0),
        center_of_mass_ready(false),
		cumulative_mass_ready(false){
       // Standardinitialisierung der Felder
}

QuadtreeNode::~QuadtreeNode(){
        // Speicher der Unterknoten freigeben
        for (auto* child : children) {
            delete child; // Dynamischen Speicher freigeben
        }
        children.clear(); // Sicherstellen, dass keine ungültigen Zeiger verbleiben
}

Vector2d<double> QuadtreeNode::calculate_node_center_of_mass(){
            if (children.empty()) { // Blattknoten
                center_of_mass_ready = true;
                return center_of_mass; // Rückgabe der gespeicherten Position
            }

            Vector2d<double> total_center(0.0, 0.0);
            cumulative_mass = 0.0;

            for (auto* child : children) {
                double child_mass = child->calculate_node_cumulative_mass();
                Vector2d<double> child_center = child->calculate_node_center_of_mass();

                total_center += child_center * child_mass;
                cumulative_mass += child_mass;
            }

            if (cumulative_mass > 0) {
                center_of_mass = total_center / cumulative_mass; // Gewichteter Mittelwert
            }

            center_of_mass_ready = true;
            return center_of_mass;
}