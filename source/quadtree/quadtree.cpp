#include "quadtree.h"
#include "quadtreeNode.h"
#include <set>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

Quadtree::Quadtree(Universe& universe, BoundingBox bounding_box, std::int8_t construct_mode) {
    root = new QuadtreeNode(bounding_box);
	std::vector<std::int32_t> vec;
    switch (construct_mode) {
    case 0:
        construct(universe, bounding_box, vec);
        break;
    case 1:
        construct_task(universe, bounding_box, vec);
        break;
    case 2:
        construct_task_with_cutoff(universe, bounding_box, vec);
        break;
    default:
        throw std::invalid_argument("Invalid construct_mode");
    }
}

Quadtree::~Quadtree() {
  delete root;
  root = nullptr;
}

void Quadtree::calculate_cumulative_masses() {
    root->calculate_node_cumulative_mass();
}

void Quadtree::calculate_center_of_mass() {
    root->calculate_node_center_of_mass();
}


std::vector<QuadtreeNode*> Quadtree::construct(Universe& universe, BoundingBox BB, std::vector<std::int32_t> body_indices) {

    // Basisfall: Kein Körper im Bereich
    if (body_indices.empty()) {
        return {};
    }

    // Basisfall: Genau ein Körper -> Blattknoten
    if (body_indices.size() == 1) {
        QuadtreeNode* leaf_node = new QuadtreeNode(BB);
        int body_index = body_indices[0];
        leaf_node->body_identifier = body_index;
        leaf_node->cumulative_mass = universe.weights[body_index];
        leaf_node->center_of_mass = universe.positions[body_index];
        leaf_node->cumulative_mass_ready = true;
        leaf_node->center_of_mass_ready = true;
        return {leaf_node}; // Direkt zurückgeben
    }

    // Unterteile BoundingBox in 4 Subquadranten
    std::vector<BoundingBox> sub_boxes = BB.subdivide();
    std::vector<QuadtreeNode*> children_nodes;

    for (const auto& sub_box : sub_boxes) {
        std::vector<int32_t> sub_indices;

        // Verteile die Körper auf die Subquadranten
        for (int32_t body_index : body_indices) {
            if (sub_box.contains(universe.positions[body_index])) {
                sub_indices.push_back(body_index);
            }
        }

        // Rekursiv weiter für jeden Subquadranten
        if (!sub_indices.empty()) {
            QuadtreeNode* child_node = new QuadtreeNode(sub_box);
            auto sub_children = construct(universe, sub_box, sub_indices);
            child_node->children = sub_children;
            children_nodes.push_back(child_node);
        }
    }

    return children_nodes; // Rückgabe aller Subknoten

	return std::vector<QuadtreeNode*>();

}

std::vector<QuadtreeNode*> Quadtree::construct_task(Universe& universe, BoundingBox BB, std::vector<std::int32_t> body_indices) {
    return std::vector<QuadtreeNode*>();
}

std::vector<QuadtreeNode*> Quadtree::construct_task_with_cutoff(Universe& universe, BoundingBox& BB, std::vector<std::int32_t>& body_indices) {
    return std::vector<QuadtreeNode*>();
}

std::vector<BoundingBox> Quadtree::get_bounding_boxes(QuadtreeNode* qtn) {
    // traverse quadtree and collect bounding boxes
    std::vector<BoundingBox> result;
    // collect bounding boxes from children
    for(auto child: qtn->children){
        for(auto bb: get_bounding_boxes(child)){
            result.push_back(bb);
        }
    }
    result.push_back(qtn->bounding_box);
    return result;
}








