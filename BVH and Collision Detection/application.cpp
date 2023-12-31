// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
// 
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#include "application.hpp"

/**
 * This method will construct a binary bounding volume hierarchy (BVH) tree from the set of triangles to the given
 * depth using top to bottom approach. The method should output the root node of the computed BVH.
 *
 * @param 	triangles	The list of triangles.
 * @param 	depth	 	The maximum depth the binary tree should have.
 * @param   min_triangles   The minimum number of triangles for which we can consider to create child nodes.
 * @return	The method should output the root node of the computed BVH.
 */
BVHNode* Application::construct(std::vector<Triangle*> triangles, int max_depth, int min_triangles_for_split) {

    glm::vec4 minPoint = triangles.at(0)->v1;
    glm::vec4 maxPoint = minPoint;    

    // setup min and max points of given set of triangles
    for ( Triangle* tr : triangles )
    {
        for ( glm::vec4 vertex : tr->getVertices() )
        {
            minPoint.x = std::min(vertex.x, minPoint.x);
            minPoint.y = std::min(vertex.y, minPoint.y);
            minPoint.z = std::min(vertex.z, minPoint.z);

            maxPoint.x = std::max(vertex.x, maxPoint.x);
            maxPoint.y = std::max(vertex.y, maxPoint.y);
            maxPoint.z = std::max(vertex.z, maxPoint.z);
        }
    }

    // create new AABB (Axis Aligned Bounding Box)
    auto node = new BVHNode(minPoint, maxPoint, triangles);

    // triangles split into left and right halves
    std::vector<Triangle*> leftTriangles;
    std::vector<Triangle*> rightTriangles;

    // determine the longest edge along one of the axis
    bool isXLongest, isYLongest, isZLongest;
    isXLongest = isYLongest = isZLongest = false;

    float xLength = std::abs(maxPoint.x - minPoint.x);
    float yLength = std::abs(maxPoint.y - minPoint.y);
    float zLength = std::abs(maxPoint.z - minPoint.z);

    float splitCoord;

    // determine in which axis AABB's longest edge is
    if ( xLength >= yLength && xLength >= zLength )
    {
        splitCoord = (minPoint.x + maxPoint.x) / 2.0f;
        isXLongest = true;
    }
    else if ( yLength >= xLength && yLength >= zLength )
    {
        splitCoord = (minPoint.y + maxPoint.y) / 2.0f;
        isYLongest = true;
    }
    else
    {
        splitCoord = (minPoint.z + maxPoint.z) / 2.0f;
        isZLongest = true;
    }

    float firstVertexCoord;
    float secondVertexCoord;
    float thirdVertexCoord;

    // split triangles along the plane
    for ( Triangle* tr : triangles )
    {
        if ( isXLongest )
        {
            firstVertexCoord = tr->v1.x;
            secondVertexCoord = tr->v2.x;
            thirdVertexCoord = tr->v3.x;
        }
        else if ( isYLongest )
        {
            firstVertexCoord = tr->v1.y;
            secondVertexCoord = tr->v2.y;
            thirdVertexCoord = tr->v3.y;
        }
        else 
        {
            firstVertexCoord = tr->v1.z;
            secondVertexCoord = tr->v2.z;
            thirdVertexCoord = tr->v3.z;
        }

        // all triangle vertices to the left of the split plane
        if ( firstVertexCoord <= splitCoord && secondVertexCoord <= splitCoord && thirdVertexCoord <= splitCoord )
        {
            leftTriangles.push_back(tr);
        }
        // all triangle vertices to the right of the split plane
        else if ( firstVertexCoord > splitCoord && secondVertexCoord > splitCoord && thirdVertexCoord > splitCoord )
        {
            rightTriangles.push_back(tr);
        }
        // else triangle intersects the split plane
        else
        {
            float minDistance = std::min({firstVertexCoord, secondVertexCoord, thirdVertexCoord});
            float maxDistance = std::max({firstVertexCoord, secondVertexCoord, thirdVertexCoord});

            // triangle lies mostly to the left of the split plane
            if ( splitCoord - minDistance >= maxDistance - splitCoord )
            {
                leftTriangles.push_back(tr);
            }
            // triangle lies mostly to the right of the split plane
            else
            {
                rightTriangles.push_back(tr);
            }
        }
    }

    // check if both potential children can be split further
    if ( leftTriangles.size() > 0 && rightTriangles.size() > 0 && 
         node->get_depth() <= max_depth )
    {
        if ( leftTriangles.size() >= min_triangles_for_split )
        {
            node->set_left( Application::construct(leftTriangles, max_depth-1, min_triangles_for_split) );
        }
        else
        {
            node->set_left( Application::construct(leftTriangles, 0, min_triangles_for_split) );
        }

        if ( rightTriangles.size() >= min_triangles_for_split )
        {
            node->set_right( Application::construct(rightTriangles, max_depth-1, min_triangles_for_split) );
        }
        else
        {
            node->set_right( Application::construct(rightTriangles, 0, min_triangles_for_split) );
        }
    }

    return node;
}

/**
 * This method gets two BVH trees and tests what nodes and their respective triangles are in collision.
 *
 * @param 	first_node	  The first BVH root.
 * @param 	first_matrix  The model matrix applied to the first model.
 * @param   second_node   The second BVH root.
 * @param   second_matrix The model matrix applied to the second model.
 * @return	The method does not return anything, you should mark the intersecting nodes and triangles directly.
 */
void Application::test_collision(BVHNode& first_node, const glm::mat4& first_matrix, BVHNode& second_node,
                                 const glm::mat4& second_matrix) {

    // points defining AABBs - multiplied by model matrices -> they are comparable
	// transformation from local space to world space
    glm::vec4 firstMin = first_matrix * first_node.get_min();
    glm::vec4 firstMax = first_matrix * first_node.get_max();
    
    glm::vec4 secondMin = second_matrix * second_node.get_min();
    glm::vec4 secondMax = second_matrix * second_node.get_max();

    // check collision of root AABBs
    if ( !( firstMax.x >= secondMin.x && firstMin.x <= secondMax.x ) ||
         !( firstMax.y >= secondMin.y && firstMin.y <= secondMax.y ) ||
         !( firstMax.z >= secondMin.z && firstMin.z <= secondMax.z )
       )
    {
        // no collision 
        return;
    }
    // collision detected
    else
    {
        // mark nodes as colliding
        first_node.collision = true;
        second_node.collision = true;

        // if both nodes are leaves - check triangles
        if ( &first_node.get_left() == nullptr && &second_node.get_left() == nullptr )
        {
            std::vector<Triangle*> first_triangles = first_node.get_triangles();
            std::vector<Triangle*> second_triangles = second_node.get_triangles();

            // check all triangles for collision
            for ( size_t i = 0; i < first_triangles.size(); ++i )
            {
                for ( size_t j = 0; j < second_triangles.size(); ++j )
                {
                    if ( triangle_triangle_intersection( *first_triangles.at(i), first_matrix, *second_triangles.at(j), second_matrix ) )
                    {
                        first_triangles.at(i)->collision = true;
                        second_triangles.at(j)->collision = true;
                    }
                }
            }
        }
        // either one or both nodes have children - compare them all against each other
        else
        {
            // first node is a leaf
            if ( &first_node.get_left() == nullptr )
            {
                test_collision(first_node, first_matrix, second_node.get_left(), second_matrix);
                test_collision(first_node, first_matrix, second_node.get_right(), second_matrix);
            }
            // second node is a leaf
            else if ( &second_node.get_left() == nullptr )
            {
                test_collision(first_node.get_left(), first_matrix, second_node, second_matrix);
                test_collision(first_node.get_right(), first_matrix, second_node, second_matrix);
            }
            // neither node is a leaf
            else 
            {
                test_collision(first_node.get_left(), first_matrix, second_node.get_left(), second_matrix);
                test_collision(first_node.get_left(), first_matrix, second_node.get_right(), second_matrix);
                test_collision(first_node.get_right(), first_matrix, second_node.get_left(), second_matrix);
                test_collision(first_node.get_right(), first_matrix, second_node.get_right(), second_matrix);
            }
        }
    }
}