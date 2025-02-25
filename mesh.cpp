#include "mesh.h"

#include <iostream>
#include <fstream>

#include <QFileInfo>
#include <QString>
#include <cassert>
#include <set>
#include <map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "util/tiny_obj_loader.h"

using namespace Eigen;
using namespace std;

void Mesh::initFromVectors(const vector<Vector3f> &vertices,
                           const vector<Vector3i> &faces)
{
    // Copy vertices and faces into internal vector
    _vertices = vertices;
    _faces    = faces;

    buildHalfedges();
}

void Mesh::loadFromFile(const string &filePath)
{
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    QFileInfo info(QString(filePath.c_str()));
    string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                                info.absoluteFilePath().toStdString().c_str(), (info.absolutePath().toStdString() + "/").c_str(), true);
    if (!err.empty()) {
        cerr << err << endl;
    }

    if (!ret) {
        cerr << "Failed to load/parse .obj file" << endl;
        return;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            unsigned int fv = shapes[s].mesh.num_face_vertices[f];

            Vector3i face;
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                face[v] = idx.vertex_index;
            }
            _faces.push_back(face);

            index_offset += fv;
        }
    }
    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        _vertices.emplace_back(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    }

    buildHalfedges();

    cout << "Loaded " << _faces.size() << " faces and " << _vertices.size() << " vertices" << endl;
}

void Mesh::saveToFile(const string &filePath)
{
    exportHalfedges();

    ofstream outfile;
    outfile.open(filePath);

    // Write vertices
    for (size_t i = 0; i < _vertices.size(); i++)
    {
        const Vector3f &v = _vertices[i];
        outfile << "v " << v[0] << " " << v[1] << " " << v[2] << endl;
    }

    // Write faces
    for (size_t i = 0; i < _faces.size(); i++)
    {
        const Vector3i &f = _faces[i];
        outfile << "f " << (f[0]+1) << " " << (f[1]+1) << " " << (f[2]+1) << endl;
    }

    outfile.close();
}

void Mesh::buildHalfedges() {
    std::vector<Vertex*> v_list;
    std::map<std::pair<int, int>, Edge*> e_list;

    for (Eigen::Vector3f &vpoint : _vertices) {
        Vertex *v = new Vertex;
        v->pos = vpoint;
        v_list.push_back(v);
    }

    for(Eigen::Vector3i fvertices : _faces)
    {
        int v1 = fvertices[0];
        int v2 = fvertices[1];
        int v3 = fvertices[2];

        Face *f = new Face;
        Halfedge *he1 = new Halfedge /*1 to 2*/, *he2 = new Halfedge /*2 to 3*/, *he3 = new Halfedge /*3 to 1*/;
        Edge *e1 = new Edge /*1 to 2*/, *e2 = new Edge/*2 to 3*/, *e3 = new Edge /*3 to 1*/;
        he1->face = f;
        he2->face = f;
        he3->face = f;
        f->halfedge = he1;
        if(e_list.find(std::make_pair(v1,v2)) == e_list.end()) {
            e_list[std::make_pair(v1,v2)] = e1;
            e_list[std::make_pair(v2,v1)] = e1;
        } else {
            e1 = e_list[std::make_pair(v1,v2)];
            he1->twin = e1->halfedge;
            he1->twin->twin = he1;
        }
        if(e_list.find(std::make_pair(v2,v3)) == e_list.end()) {
            e_list[std::make_pair(v2,v3)] = e2;
            e_list[std::make_pair(v3,v2)] = e2;
        } else {
            e2 = e_list[std::make_pair(v2,v3)];
            he2->twin = e2->halfedge;
            he2->twin->twin = he2;
        }
        if(e_list.find(std::make_pair(v3,v1)) == e_list.end()) {
            e_list[std::make_pair(v3,v1)] = e3;
            e_list[std::make_pair(v1,v3)] = e3;
        } else {
            e3 = e_list[std::make_pair(v3,v1)];
            he3->twin = e3->halfedge;
            he3->twin->twin = he3;
        }
        he1->edge = e1;
        he2->edge = e2;
        he3->edge = e3;

        e1->halfedge = he1;
        e2->halfedge = he2;
        e3->halfedge = he3;

        v_list[v1]->halfedge = he1;
        v_list[v2]->halfedge = he2;
        v_list[v3]->halfedge = he3;

        he1->vertex = v_list[v1];
        he2->vertex = v_list[v2];
        he3->vertex = v_list[v3];

        he1->next = he2;
        he2->next = he3;
        he3->next = he1;

        _halfedges.push_back(he1);
        _halfedges.push_back(he2);
        _halfedges.push_back(he3);
    }
}

void Mesh::exportHalfedges() {
    std::map<Vertex*, int> v_list;
    std::set<Face*> seen;
    int v_idx = 0;

    _vertices.clear();
    _faces.clear();

    for(Halfedge *h : _halfedges) {
        if(seen.contains(h->face)) {continue;}
        seen.insert(h->face);

        Vertex *v1 = h->vertex;
        Vertex *v2 = h->next->vertex;
        Vertex *v3 = h->next->next->vertex;

        if(!v_list.contains(v1)) {
            v_list[v1] = v_idx;
            v_idx++;
            _vertices.push_back(v1->pos);
        }

        if(!v_list.contains(v2)) {
            v_list[v2] = v_idx;
            v_idx++;
            _vertices.push_back(v2->pos);
        }

        if(!v_list.contains(v3)) {
            v_list[v3] = v_idx;
            v_idx++;
            _vertices.push_back(v3->pos);
        }

        _faces.push_back(Eigen::Vector3i(v_list[v1], v_list[v2], v_list[v3]));
    }
}

void validate(const Mesh &mesh) {
    const std::vector<Halfedge*> halfedges = mesh.getHalfedges();

    // Tests 0-4 : half edges have all fields
    for(Halfedge *h : halfedges) {
        assert(h != nullptr);
        assert(h->edge != nullptr);
        assert(h->edge->halfedge != nullptr);

        assert(h->face != nullptr);
        assert(h->face->halfedge != nullptr);

        assert(h->next != nullptr);

        assert(h->twin != nullptr);

        assert(h->vertex != nullptr);
        assert(h->vertex->halfedge != nullptr);
    }

    // Test 5 : each half edge has two edges
    std::map<Edge*, int> seen;
    for(Halfedge *h : halfedges) {
        seen[h->edge] += 1;
    }

    for (const auto& pair : seen) {
        assert(pair.second == 2);
    }

    // Test 6 : we can follow halfedges in a loop around a face back to the original one
    for (Halfedge *h : halfedges) {
        assert(h->next->next->next == h);
    }

    // Test 7 : halfedges are twins of each other
    for (Halfedge *h : halfedges) {
        assert(h->twin->twin == h);
    }

    // Test 8 : follow the halfedges around a vertex, they should share that vertex
    for (Halfedge *h : halfedges) {
        Vertex *v = h->vertex;
        Halfedge *start = h;
        do {
            assert(h->vertex == v);
            h = h->twin->next;
        }
        while(h != start);
    }

    // Test 9 : we have a disc around a vertex
    // (if we follow the disc around that vertex, we see all of the halfedges that originate from it)
    std::map<Vertex*, std::set<Halfedge*>> neighborhoods;
    for(Halfedge *h : halfedges) {
        neighborhoods[h->vertex].insert(h);
    }

    for (Halfedge *h : halfedges) {
        std::set<Halfedge*> neighborhood;

        Vertex *v = h->vertex;
        Halfedge *start = h;
        do {
            neighborhood.insert(h);
            h = h->twin->next;
        }
        while(h != start);

        assert(neighborhoods[v] == neighborhood);
    }

    // Test 10 : halfedges of same face share that face
    for (Halfedge *h : halfedges) {
        Face *f = h->face;
        assert(h->next->face == f);
        assert(h->next->next->face == f);
    }
}
