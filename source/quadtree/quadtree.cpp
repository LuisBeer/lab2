#include "quadtree.h"
#include "quadtreeNode.h"
#include <set>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

Quadtree::Quadtree(Universe& universe, BoundingBox bounding_box, std::int8_t construct_mode) {
    root = new QuadtreeNode(bounding_box);
    std::vector<int> indices;
    for (int i = 0; i < universe.num_bodies; ++i) {
        indices.push_back(i);
    }
    switch (construct_mode) {
    case 0:
        root->children = construct(universe, bounding_box, indices);
        break;
    case 1:
        root->children = construct_task(universe, bounding_box, indices);
        break;
    case 2:
        root->children = construct_task_with_cutoff(universe, bounding_box, indices);
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
        //throw std::invalid_argument("Invalid body indices");
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

    double xmid = BB.x_min + (BB.x_max - BB.x_min)/2;
    double ymid = BB.y_min + (BB.y_max - BB.y_min)/2;
    const BoundingBox Q1 = BoundingBox(BB.x_min, xmid, ymid, BB.y_max);
    const BoundingBox Q2 = BoundingBox(xmid, BB.x_max, ymid, BB.y_max);
    const BoundingBox Q3 = BoundingBox(BB.x_min, xmid, BB.y_min, ymid);
    const BoundingBox Q4 = BoundingBox(xmid, BB.x_max, BB.y_min ,ymid);
    std::vector<BoundingBox> sub_boxes = {Q1, Q2, Q3, Q4};

    std::vector<QuadtreeNode*> children_nodes;

    for (auto& sub_box : sub_boxes) {
        std::vector<int32_t> sub_indices;

        // Verteile die Körper auf die Subquadranten
        for (int32_t body_index : body_indices) {
            if (sub_box.contains(universe.positions[body_index])) {
                sub_indices.push_back(body_index);
            }
        }

        // Rekursiv weiter für jeden Subquadranten
        if (!sub_indices.empty()) {
            //Kind ist ein Blattknoten
            if(sub_indices.size() == 1) {
                QuadtreeNode* leaf_node = new QuadtreeNode(sub_box);
                int body_index = sub_indices[0];
                leaf_node->body_identifier = body_index;
                leaf_node->cumulative_mass = universe.weights[body_index];
                leaf_node->center_of_mass = universe.positions[body_index];
                leaf_node->cumulative_mass_ready = true;
                leaf_node->center_of_mass_ready = true;
                children_nodes.push_back(leaf_node);
            } else { //Kind hat mehr als einen Himmelskörper und ist somit kein Blatt
                QuadtreeNode* child_node = new QuadtreeNode(sub_box);
                auto sub_children = construct(universe, sub_box, sub_indices);
                child_node->children = sub_children;
                children_nodes.push_back(child_node);
            }
        }
    }

    return children_nodes; // Rückgabe aller Subknoten
}

std::vector<QuadtreeNode*> Quadtree::construct_task(Universe& universe, BoundingBox BB, std::vector<std::int32_t> body_indices) {
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

    double xmid = BB.x_min + (BB.x_max - BB.x_min)/2;
    double ymid = BB.y_min + (BB.y_max - BB.y_min)/2;
    const BoundingBox Q1 = BoundingBox(BB.x_min, xmid, ymid, BB.y_max);
    const BoundingBox Q2 = BoundingBox(xmid, BB.x_max, ymid, BB.y_max);
    const BoundingBox Q3 = BoundingBox(BB.x_min, xmid, BB.y_min, ymid);
    const BoundingBox Q4 = BoundingBox(xmid, BB.x_max, BB.y_min ,ymid);
    std::vector<BoundingBox> sub_boxes = {Q1, Q2, Q3, Q4};

    std::vector<QuadtreeNode*> children_nodes;

    #pragma omp parallel
    {
        // Liste für lokal generierte Knoten
        std::vector<QuadtreeNode*> local_children;

        #pragma omp single
        {
            for (auto& sub_box : sub_boxes) {
                std::vector<int32_t> sub_indices;

                // Verteile die Körper auf die Subquadranten
                for (int32_t body_index : body_indices) {
                    if (sub_box.contains(universe.positions[body_index])) {
                        sub_indices.push_back(body_index);
                    }
                }

                // Erzeuge Tasks für jeden Subquadranten
                if (!sub_indices.empty()) {
                    #pragma omp task shared(local_children)
                    {
                        QuadtreeNode* child_node = new QuadtreeNode(sub_box);
                        auto sub_children = construct_task(universe, sub_box, sub_indices);
                        child_node->children = sub_children;

                        #pragma omp critical
                        local_children.push_back(child_node);
                    }
                }
            }
        }

        #pragma omp critical
        children_nodes.insert(children_nodes.end(), local_children.begin(), local_children.end());
    }

    return children_nodes;
}

std::vector<QuadtreeNode*> Quadtree::construct_task_with_cutoff(Universe& universe, BoundingBox& BB, std::vector<std::int32_t>& body_indices) {
  const int cutoff = 100;
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

    double xmid = BB.x_min + (BB.x_max - BB.x_min)/2;
    double ymid = BB.y_min + (BB.y_max - BB.y_min)/2;
    const BoundingBox Q1 = BoundingBox(BB.x_min, xmid, ymid, BB.y_max);
    const BoundingBox Q2 = BoundingBox(xmid, BB.x_max, ymid, BB.y_max);
    const BoundingBox Q3 = BoundingBox(BB.x_min, xmid, BB.y_min, ymid);
    const BoundingBox Q4 = BoundingBox(xmid, BB.x_max, BB.y_min ,ymid);
    std::vector<BoundingBox> sub_boxes = {Q1, Q2, Q3, Q4};

    std::vector<QuadtreeNode*> children_nodes;

	#pragma omp parallel
   	 {
        #pragma omp single
        {
            for (auto& sub_box : sub_boxes) {
                std::vector<int32_t> sub_indices;

                // Verteile die Körper auf die Subquadranten
                for (int32_t body_index : body_indices) {
                    if (sub_box.contains(universe.positions[body_index])) {
                        sub_indices.push_back(body_index);
                    }
                }

                // Fall 1: Wenige Körper -> Serielle Konstruktion
                if (!sub_indices.empty() && sub_indices.size() <= cutoff) {
                    QuadtreeNode* child_node = new QuadtreeNode(sub_box);
                    auto sub_children = construct(universe, sub_box, sub_indices);
                    child_node->children = sub_children;

                    #pragma omp critical
                    children_nodes.push_back(child_node);
                }

                // Fall 2: Viele Körper -> Parallelisierte Konstruktion
                if (!sub_indices.empty() && sub_indices.size() > cutoff) {
                    #pragma omp task shared(children_nodes)
                    {
                        QuadtreeNode* child_node = new QuadtreeNode(sub_box);
                        auto sub_children = construct_task_with_cutoff(universe, sub_box, sub_indices);
                        child_node->children = sub_children;

                        #pragma omp critical
                        children_nodes.push_back(child_node);
                    }
                }
            }
        }
    }

    return children_nodes;
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








