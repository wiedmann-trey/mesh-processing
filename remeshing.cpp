#include "mesh.h"
#include <unordered_set>
#include <iostream>

Eigen::Vector3f triangleNormal(Eigen::Vector3f a, Eigen::Vector3f b, Eigen::Vector3f c) {
    Eigen::Vector3f AB = b-a;
    Eigen::Vector3f AC = c-a;
    return AB.cross(AC).normalized();
}

float triangleArea(Eigen::Vector3f a, Eigen::Vector3f b, Eigen::Vector3f c) {
    Eigen::Vector3f AB = b-a;
    Eigen::Vector3f AC = c-a;
    return .5 * AB.cross(AC).norm();
}

float region_area(Eigen::Vector3f a, Eigen::Vector3f b, Eigen::Vector3f c) {
    Eigen::Vector3f e1 = (b-a);
    Eigen::Vector3f e2 = (c-a);

    if(triangleArea(a,b,c) < 1e-8) return 0;

    float cos_alpha = (-e1).dot(c-b) / (e1.norm() * (c-b).norm());
    float cos_beta = (-e2).dot(b-c) / (e2.norm() * (b-c).norm());

    float sin_alpha = std::sqrt(1-cos_alpha*cos_alpha);
    float sin_beta = std::sqrt(1-cos_beta*cos_beta);

    float cot_alpha = std::max(0.f,cos_alpha/sin_alpha);
    float cot_beta = std::max(0.f, cos_beta/sin_beta);

    float cos_gamma = e1.dot(e2) / (e1.norm() * e2.norm());
    float gamma = std::acos(std::clamp(cos_gamma, -1.0f, 1.0f));

    if(gamma > (std::numbers::pi / 2)) return triangleArea(a, b, c) / 2;

    float v_area = (cot_alpha * e2.norm() * e2.norm() + cot_beta * e1.norm() * e1.norm())/8;

    return v_area;
}

float area(Vertex * v) {
    float a = 0;
    Halfedge *h = v->halfedge;
    do {
        a += region_area(h->vertex->pos, h->next->vertex->pos, h->next->next->vertex->pos);
        h = h->twin->next;
    }
    while(h != v->halfedge);

    return a;
}

void Mesh::remesh_iteration(float damping) {
    // unordered map of edge to length
    std::unordered_map<Edge*, float> lengths;
    float avg_length = 0;
    int n_edges = 0;

    // for each edge
    // compute length of edge
    // compute average
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!lengths.contains(h->edge)) {
            lengths[h->edge] = (h->vertex->pos - h->twin->vertex->pos).norm();
            avg_length += lengths[h->edge];
            n_edges++;
        }
    }

    avg_length /= n_edges;

    std::unordered_set<Edge*> deleted;

    for(auto &pair : lengths) {
        Edge *e = pair.first;
        if(!deleted.contains(e)) {
            if(pair.second > (4.f/3)*avg_length) {
                edgeSplit(e->halfedge);
            } else if(pair.second < (4.f/5)*avg_length) {
                /*if(canCollapse(e->halfedge)) {
                    deleted.insert(e->halfedge->next->edge);
                    deleted.insert(e->halfedge->twin->next->next->edge);
                    deleted.insert(e);
                    edgeCollapse(e->halfedge);
                }*/
            }
        }
    }

    // for each edge
    // check degrees and flip if flip is better
    std::unordered_set<Edge*> seen;
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!seen.contains(h->edge)) {
            seen.insert(h->edge);
            int old_a = degree(h->vertex);
            int old_b = degree(h->twin->vertex);
            int old_c = degree(h->next->next->vertex);
            int old_d = degree(h->twin->next->next->vertex);
            int old_deviation = std::abs(old_a-6) + std::abs(old_b-6) + std::abs(old_c-6) + std::abs(old_d-6);

            int new_a = old_a-1;
            int new_b = old_b-1;
            int new_c = old_c+1;
            int new_d = old_d+1;
            int new_deviation = std::abs(new_a-6) + std::abs(new_b-6) + std::abs(new_c-6) + std::abs(new_d-6);

            if(new_deviation < old_deviation) {
                edgeFlip(h);
            }
        }
    }

    // calculate voronoi areas
    std::unordered_map<Vertex*, float> areas;
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!areas.contains(h->vertex)) {
            areas[h->vertex] = area(h->vertex);
        }
    }

    // find new positions
    std::unordered_map<Vertex*, Eigen::Vector3f> new_positions;
    std::unordered_map<Vertex*, Eigen::Vector3f> normal_vectors;
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!new_positions.contains(h->vertex)) {
            Halfedge *start = h;
            float factor = 0;
            Eigen::Vector3f centroid = Eigen::Vector3f(0,0,0);
            int n_faces = 0;
            Eigen::Vector3f normal = Eigen::Vector3f(0,0,0);
            do {
                factor += areas[h->twin->vertex];
                centroid += areas[h->twin->vertex] * h->twin->vertex->pos;
                n_faces++;
                normal += triangleNormal(h->vertex->pos, h->next->vertex->pos, h->next->next->vertex->pos);
                h = h->twin->next;
            }
            while(h != start);
            new_positions[h->vertex] = centroid/factor;
            normal_vectors[h->vertex] = normal/n_faces;
        }
    }

    // move towards new position
    for(auto &pair : new_positions) {
        Vertex *v = pair.first;
        Eigen::Vector3f pos = v->pos;
        Eigen::Vector3f n = normal_vectors[v];
        Eigen::Vector3f new_pos = pair.second;
        v->pos = pos + damping*(Eigen::Matrix3f::Identity() - n*n.transpose())*(new_pos-pos);
    }
}

void Mesh::remesh(int n, float damping) {
    for(int i = 0; i < n; i++) {
        remesh_iteration(damping);
    }
}

