#include "mesh.h"

float vertex_weight(int n) {
    if (n == 3) return 3/16.f;

    float w = (3/8.f) + (1/4.f)*std::cos(2.f*std::numbers::pi/n);
    w = (5/8.f) - w*w;
    return (1.f/n)*w;
}

void Mesh::loopSubdivide() {
    // build list of all the current edges
    // build list of all the current vertices
    // for each current vertex, store positions of adjacent ones
    // for each edge, store the surrounding 4 vertices's positions
    std::unordered_map<Vertex*, std::vector<Eigen::Vector3f>> old_vertices;
    std::unordered_map<Edge*, std::vector<Eigen::Vector3f>> old_edges;
    std::unordered_map<Vertex*, std::vector<Eigen::Vector3f>> new_vertices;

    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;

        if(!old_vertices.contains(h->vertex)) {
            Vertex *v = h->vertex;
            Halfedge *start = h;
            do {
                old_vertices[v].push_back(h->twin->vertex->pos);
                h = h->twin->next;
            }
            while(h != start);
        }

        if(!old_edges.contains(h->edge)) {
            old_edges[h->edge].push_back(h->vertex->pos);
            old_edges[h->edge].push_back(h->twin->vertex->pos);
            old_edges[h->edge].push_back(h->next->next->vertex->pos);
            old_edges[h->edge].push_back(h->twin->next->next->vertex->pos);
        }
    }

    // for each edge, split and save a list of new vertices with their 4 surrounding positions
    for(auto &pair : old_edges) {
        Edge *e = pair.first;
        Vertex *new_v = edgeSplit(e->halfedge);
        new_vertices[new_v] = pair.second;
    }

    // flip new edges that touch a new and old vertex
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;

        if(h->edge->is_new) {
            if((new_vertices.contains(h->vertex) && old_vertices.contains(h->twin->vertex)) || (new_vertices.contains(h->twin->vertex) && old_vertices.contains(h->vertex))) {
                h->edge->is_new = false;
                edgeFlip(h);
            }
        }
    }

    // now, loop each current and new vertex and update positions
    for(auto &pair : new_vertices) {
        Vertex *v = pair.first;
        std::vector<Eigen::Vector3f> surrounding = pair.second;

        v->pos = (3/8.f)*(surrounding[0] + surrounding[1]) + (1/8.f)*(surrounding[2]+surrounding[3]);
    }

    for(auto &pair : old_vertices) {
        Vertex *v = pair.first;
        std::vector<Eigen::Vector3f> surrounding = pair.second;
        int n = surrounding.size();
        float u = vertex_weight(n);

        Eigen::Vector3f new_pos = Eigen::Vector3f(0,0,0);
        for(Eigen::Vector3f vert : surrounding) {
            new_pos += u*vert;
        }
        new_pos+=(1-n*u)*v->pos;
        v->pos = new_pos;
    }
}

void Mesh::loopSubdivision(int n) {
    for(int i = 0; i < n; i++) {
        loopSubdivide();
    }
}
