#include "quadtreeNode.h"

#include <iostream>


double QuadtreeNode::calculate_node_cumulative_mass(){
    return 0.0;
}

QuadtreeNode::QuadtreeNode(BoundingBox arg_bounding_box){
}

QuadtreeNode::~QuadtreeNode(){
}

Vector2d<double> QuadtreeNode::calculate_node_center_of_mass(){
    return Vector2d<double>(0.0, 0.0);
}