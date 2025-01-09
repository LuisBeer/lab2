#include "quadtree.h"
#include "quadtreeNode.h"
#include <set>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

Quadtree::Quadtree(Universe& universe, BoundingBox bounding_box, std::int8_t construct_mode){
    root = new QuadtreeNode(bounding_box);

    switch (construct_mode) {
        case 0:
            construct(universe, bounding_box, {}); // Implementiere seriellen Aufbau
        break;
        case 1:
            construct_task(universe, bounding_box, {}); // Parallel ohne Cutoff
        break;
        case 2:
            construct_task_with_cutoff(universe, bounding_box, {}); // Parallel mit Cutoff
        break;
        default:
            throw std::invalid_argument("Invalid construct_mode");
    }
}

Quadtree::~Quadtree(){
  delete root;
  root = nullptr;
}

void Quadtree::calculate_cumulative_masses(){
    root->calculate_node_cumulative_mass();
}

void Quadtree::calculate_center_of_mass(){
    root->calculate_node_center_of_mass();
}


std::vector<QuadtreeNode*> Quadtree::construct(Universe& universe, BoundingBox BB, std::vector<std::int32_t> body_indices){

    //TODO implementieren 3d

	 // Erstelle den Wurzelknoten des Quadtrees
  QuadtreeNode* root = new QuadtreeNode();
  root->bounding_box = BB;
  root->center_of_mass_ready = false;
  root->cumulative_mass_ready = false;
  root->body_identifier = -1;

  // Teile den Raum des Universums in vier Subquadranten auf
  std::vector<QuadtreeNode*> subquadranten;
  for (int i = 0; i < 4; i++) {
    BoundingBox sub_BB = BB.getSubBoundingBox(i);
    std::vector<int> sub_body_indices;

    // Finde die Körper, die in dem aktuellen Subquadranten liegen
    for (int j = 0; j < body_indices.size(); j++) {
      int body_index = body_indices[j];
      if (sub_BB.contains(universe.positions[body_index])) {
        sub_body_indices.push_back(body_index);
      }
    }

    // Erstelle einen neuen Knoten für den Subquadranten
    QuadtreeNode* sub_node = new QuadtreeNode();
    sub_node->bounding_box = sub_BB;
    sub_node->center_of_mass_ready = false;
    sub_node->cumulative_mass_ready = false;
    sub_node->body_identifier = -1;

    // Wenn es sich um einen Blattknoten handelt, setze die entsprechenden Werte
    if (sub_body_indices.size() == 1) {
      sub_node->body_identifier = sub_body_indices[0];
      sub_node->center_of_mass = universe.positions[sub_body_indices[0]];
      sub_node->cumulative_mass = universe.weights[sub_body_indices[0]];
      sub_node->center_of_mass_ready = true;
      sub_node->cumulative_mass_ready = true;
    }

    // Füge den Subquadranten zum Vektor hinzu
    subquadranten.push_back(sub_node);
  }

  // Setze die direkten Subquadranten von root
  root->children = subquadranten;

    return std::vector<QuadtreeNode*>();
}

std::vector<QuadtreeNode*> Quadtree::construct_task(Universe& universe, BoundingBox BB, std::vector<std::int32_t> body_indices){
    return std::vector<QuadtreeNode*>();
}

std::vector<QuadtreeNode*> Quadtree::construct_task_with_cutoff(Universe& universe, BoundingBox& BB, std::vector<std::int32_t>& body_indices){
    return std::vector<QuadtreeNode*>();
}

std::vector<BoundingBox> Quadtree::get_bounding_boxes(QuadtreeNode* qtn){
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








