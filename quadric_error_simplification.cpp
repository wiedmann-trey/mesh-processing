#include "mesh.h"
#include <map>
#include <unordered_set>
#include <cassert>

bool canCollapseEdge(Halfedge *halfedge) {
    Halfedge *twin = halfedge->twin;

    std::unordered_set<Vertex*> hset;
    std::unordered_set<Vertex*> tset;

    Halfedge *h = halfedge;
    do {
        hset.insert(h->twin->vertex);
        h = h->twin->next;
    }
    while(h != halfedge);

    h = twin;
    do {
        tset.insert(h->twin->vertex);
        h = h->twin->next;
    }
    while(h != twin);

    for(Vertex* v : hset) {
        if(tset.contains(v) && degree(v) <= 3) return false;
    }

    return true;
}

Eigen::Vector3f computeNormal(Eigen::Vector3f a, Eigen::Vector3f b, Eigen::Vector3f c) {
    Eigen::Vector3f AB = b-a;
    Eigen::Vector3f AC = c-a;
    return AB.cross(AC).normalized();
}

std::pair<float, Eigen::Vector3f> linear_search(Eigen::Matrix4f q, Eigen::Vector3f a, Eigen::Vector3f b) {
    float best_error = std::numeric_limits<float>::infinity();
    Eigen::Vector3f best_point = a;
    int num_points = 10;
    Eigen::Vector3f step = (b-a)/num_points;
    for(int i = 0; i<= num_points; i++) {
        Eigen::Vector4f point;
        point << a + step*i, 1;
        float error = point.transpose() * q * point;
        if(error < best_error) {
            best_error = error;
            best_point = point.head<3>();
        }
    }
    return std::make_pair(best_error, best_point);
}

std::pair<float, Eigen::Vector3f> optimal_position(Eigen::Matrix4f q, Eigen::Vector3f a, Eigen::Vector3f b) {
    Eigen::Matrix4f mat_to_invert;
    mat_to_invert << q(0,0), q(0,1), q(0,2), q(0,3),
                     q(0,1), q(1,1), q(1,2), q(1,3),
                     q(0,2), q(1,2), q(2,2), q(2,3),
                     0,      0,      0,      1;
    if(mat_to_invert.determinant() == 0) {
        return linear_search(q, a, b);
    }

    Eigen::Vector4f point = mat_to_invert.inverse() * Eigen::Vector4f(0,0,0,1);
    float error = point.transpose() * q * point;
    return std::make_pair(error, point.head<3>());
}

void updateEdge(Halfedge *h, std::multimap<float, std::pair<Edge*, Eigen::Vector3f>> &pq, std::unordered_map<Edge*, std::_Rb_tree_iterator<std::pair<const float, std::pair<Edge*, Eigen::Vector3f>>>> &edge_iterators, std::unordered_map<Vertex*, Eigen::Matrix4f> &vertex_q) {
    if(canCollapseEdge(h)) {
        auto updated_error = optimal_position(vertex_q[h->vertex]+vertex_q[h->twin->vertex], h->vertex->pos, h->twin->vertex->pos);
        if(edge_iterators.contains(h->edge)) pq.erase(edge_iterators[h->edge]);
        auto it = pq.insert(std::make_pair(updated_error.first, std::make_pair(h->edge, updated_error.second)));
        edge_iterators[h->edge] = it;
    } else {
        if(edge_iterators.contains(h->edge)) {
            pq.erase(edge_iterators[h->edge]);
            edge_iterators.erase(h->edge);
        }
    }
}

void recheckCanCollapse(Vertex *v, std::multimap<float, std::pair<Edge*, Eigen::Vector3f>> &pq, std::unordered_map<Edge*, std::_Rb_tree_iterator<std::pair<const float, std::pair<Edge*, Eigen::Vector3f>>>> &edge_iterators, std::unordered_map<Vertex*, Eigen::Matrix4f> &vertex_q) {
    Halfedge *h = v->halfedge;
    do {
        updateEdge(h->next, pq, edge_iterators, vertex_q);
        h = h->twin->next;
    }
    while(h != v->halfedge);
}

void Mesh::quadricErrorSimplification(int n) {
    // go back and compute face normals
    std::unordered_map<Face*, Eigen::Vector3f> normals;
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!normals.contains(h->face)) {
            normals[h->face] = computeNormal(h->vertex->pos, h->next->vertex->pos, h->next->next->vertex->pos);
        }
    }

    // for each vertex in mesh, compute Q
    std::unordered_map<Vertex*, Eigen::Matrix4f> vertex_q;
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;
        if(!vertex_q.contains(h->vertex)) {
            Halfedge *start = h;
            Eigen::Matrix4f q = Eigen::Matrix4f::Zero();
            Eigen::Vector3f p = h->vertex->pos;
            do {
                Eigen::Vector3f normal = normals[h->face];
                float d = -p.dot(normal);
                Eigen::Matrix4f this_q;
                this_q << normal[0]*normal[0], normal[0]*normal[1], normal[0]*normal[2], normal[0]*d,
                          normal[0]*normal[1], normal[1]*normal[1], normal[1]*normal[2], normal[1]*d,
                          normal[0]*normal[2], normal[1]*normal[2], normal[2]*normal[2], normal[2]*d,
                          normal[0]*d,         normal[1]*d,         normal[2]*d,         d*d;
                q += this_q;
                h = h->twin->next;
            }
            while(h != start);
            vertex_q[start->vertex] = q;
        }
    }

    std::multimap<float, std::pair<Edge*, Eigen::Vector3f>> pq;
    std::unordered_set<Edge*> seen;
    std::unordered_map<Edge*, std::_Rb_tree_iterator<std::pair<const float, std::pair<Edge*, Eigen::Vector3f>>>> edge_iterators; // WOW templates

    // for each edge in mesh, check if it can be collapsed, and compute Q based on its vertices
    for(auto &pair : _halfedges) {
        Halfedge *h = pair.first;

        if(!seen.contains(h->edge) && canCollapse(h)) {
            std::pair<float, Eigen::Vector3f> optimize_results = optimal_position(vertex_q[h->vertex]+vertex_q[h->twin->vertex], h->vertex->pos, h->twin->vertex->pos);
            auto it = pq.insert(std::make_pair(optimize_results.first, std::make_pair(h->edge, optimize_results.second)));
            edge_iterators[h->edge] = it;
        }
        seen.insert(h->edge);
    }

    for(; n > 0; n-=2) {
        // take min of pqueue and remove
        auto best_edge_entry = pq.begin();
        if(pq.begin() == pq.end()) {
            return;
        }

        Edge *best_edge = best_edge_entry->second.first;
        Eigen::Vector3f best_pos = best_edge_entry->second.second;

        edge_iterators.erase(best_edge);
        pq.erase(best_edge_entry);

        // also need to delete the other edges being erased
        Edge *erased_1 = best_edge->halfedge->next->edge;
        Edge *erased_2 = best_edge->halfedge->twin->next->next->edge;

        if(edge_iterators.contains(erased_1)) {
            pq.erase(edge_iterators[erased_1]);
            edge_iterators.erase(erased_1);
        }

        if(edge_iterators.contains(erased_2)) {
            pq.erase(edge_iterators[erased_2]);
            edge_iterators.erase(erased_2);
        }

        // halfedge vertex is the one we keep, update its Q
        Vertex *newVert = best_edge->halfedge->vertex;
        vertex_q[newVert] = vertex_q[newVert] + vertex_q[best_edge->halfedge->twin->vertex];
        Vertex *deletedVert = best_edge->halfedge->twin->vertex;
        vertex_q.erase(deletedVert);

        Vertex *recalculate_1 = best_edge->halfedge->next->next->vertex;
        Vertex *recalculate_2 = best_edge->halfedge->twin->next->next->vertex;

        // collapse that edge
        bool collapse_check = edgeCollapse(best_edge->halfedge);
        assert(collapse_check);
        newVert->pos = best_pos;

        // recompute the Q for all edges touching the new vertex
        Halfedge *h = newVert->halfedge;
        do {
            updateEdge(h, pq, edge_iterators, vertex_q);
            h = h->twin->next;
        }
        while(h != newVert->halfedge);

        // check whether certain edges can now be collapsed and update pqueue
        recheckCanCollapse(newVert, pq, edge_iterators, vertex_q);
        recheckCanCollapse(recalculate_1, pq, edge_iterators, vertex_q);
        recheckCanCollapse(recalculate_2, pq, edge_iterators, vertex_q);
    }

}
