#pragma once

#include <vector>

#include "Eigen/StdVector"
#include "Eigen/Dense"

EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(Eigen::Matrix2f);
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(Eigen::Matrix3f);
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(Eigen::Matrix3i);

struct Halfedge;

class Mesh
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

    void initFromVectors(const std::vector<Eigen::Vector3f> &vertices,
                         const std::vector<Eigen::Vector3i> &faces);

    void loadFromFile(const std::string &filePath);
    void saveToFile(const std::string &filePath);

    const std::vector<Halfedge*> getHalfedges() const {return _halfedges;}

private:
    std::vector<Eigen::Vector3f> _vertices;
    std::vector<Eigen::Vector3i> _faces;
    std::vector<Halfedge*> _halfedges;

    void buildHalfedges();
    void exportHalfedges();
};

struct Vertex {
    Halfedge *halfedge; // half edge leaving from it
    Eigen::Vector3f pos;
};

struct Edge {
    Halfedge *halfedge;
};

struct Face {
    Halfedge *halfedge;
};

struct Halfedge {
    Halfedge *twin;
    Halfedge *next; // ccw
    Vertex *vertex; // vertex it originates from
    Edge *edge;
    Face *face;
};

void validate(const Mesh &mesh);
