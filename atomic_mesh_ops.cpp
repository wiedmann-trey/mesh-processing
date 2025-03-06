#include "mesh.h"

int degree(Vertex *v) {
    Halfedge *start = v->halfedge;
    Halfedge *h = start;
    int d = 0;
    do {
        d++;
        h = h->twin->next;
    }
    while(h != start);
    return d;
}

bool Mesh::edgeFlip(Halfedge *halfedge) {
    Halfedge *twin = halfedge->twin;

    // check endpoint degrees
    if(degree(twin->vertex) <= 3 || degree(halfedge->vertex) <= 3) {
        return false;
    }

    // reassign old vertex halfedges
    Vertex *old_halfedge_vertex = halfedge->vertex;
    Vertex *old_twin_vertex = twin->vertex;

    if(old_halfedge_vertex->halfedge == halfedge) old_halfedge_vertex->halfedge = halfedge->next->next->twin;

    if(old_twin_vertex->halfedge == twin) old_twin_vertex->halfedge = twin->next->next->twin;

    // build new triangles
    Vertex *new_halfedge_vertex = twin->next->next->vertex;
    Halfedge *new_halfedge_next = halfedge->next->next;
    Vertex *new_twin_vertex = halfedge->next->next->vertex;
    Halfedge *new_twin_next = twin->next->next;

    Halfedge *new_halfedge_previous = twin->next;
    Halfedge *new_twin_previous = halfedge->next;

    halfedge->vertex = new_halfedge_vertex;
    halfedge->next = new_halfedge_next;

    twin->vertex = new_twin_vertex;
    twin->next = new_twin_next;

    new_halfedge_previous->next = halfedge;
    new_twin_previous->next = twin;

    new_halfedge_next->next = new_halfedge_previous;
    new_twin_next->next = new_twin_previous;

    new_halfedge_previous->face = halfedge->face;
    new_twin_previous->face = twin->face;

    halfedge->face->halfedge = halfedge;
    twin->face->halfedge = twin;

    return true;
}

bool Mesh::edgeCollapse(Halfedge *halfedge) {
    Halfedge *twin = halfedge->twin;

    // check shared neighbor degrees
    if(degree(twin->next->next->vertex) <= 3 || degree(halfedge->next->next->vertex) <= 3) {
        return false;
    }

    // new vertex
    Vertex *new_vertex = halfedge->vertex;
    new_vertex->pos = (new_vertex->pos + twin->vertex->pos)/2;
    if(new_vertex->halfedge == halfedge) new_vertex->halfedge = twin->next;

    // edges to delete
    Halfedge *delete1 = halfedge->next;
    Halfedge *delete2 = twin->next->next;

    // reassign vertices for edges that have their vertex deleted
    Halfedge *start = twin;
    Halfedge *h = start;
    do {
        h->vertex = new_vertex;
        h = h->twin->next;
    }
    while(h != start);

    // reassign edges for vertices that have an edge deleted
    Vertex *v1 = halfedge->next->next->vertex;
    Vertex *v2 = twin->next->next->vertex;

    if(v1->halfedge == halfedge->next->twin) v1->halfedge = halfedge->next->next;
    if(v2->halfedge == twin->next->next) v2->halfedge = twin->next->twin;

    // faces to delete
    Face *faceDelete1 = halfedge->next->twin->face;
    Face *faceDelete2 = twin->next->next->twin->face;

    // build new face 1
    Face *face1 = halfedge->face;
    Halfedge *a1 = halfedge->next->next;
    Halfedge *a2 = halfedge->next->twin->next;
    Halfedge *a3 = a2->next;
    a1->face = face1;
    a2->face = face1;
    a3->face = face1;
    a1->next = a2;
    a2->next = a3;
    a3->next = a1;
    face1->halfedge = a1;

    // build new face 2
    Face *face2 = twin->face;
    Halfedge *b1 = twin->next;
    Halfedge *b2 = twin->next->next->twin->next;
    Halfedge *b3 = b2->next;
    b1->face = face2;
    b2->face = face2;
    b3->face = face2;
    b1->next = b2;
    b2->next = b3;
    b3->next = b1;
    face2->halfedge = b1;

    // clearing memory
    _halfedges.erase(delete1);
    _halfedges.erase(delete1->twin);
    _halfedges.erase(delete2);
    _halfedges.erase(delete2->twin);
    _halfedges.erase(halfedge);
    _halfedges.erase(twin);

    delete faceDelete1;
    delete faceDelete2;
    delete twin->vertex;
    delete delete1->edge;
    delete delete1->twin;
    delete delete1;
    delete delete2->edge;
    delete delete2->twin;
    delete delete2;
    delete halfedge->edge;
    delete twin;
    delete halfedge;

    return true;
}

Vertex *Mesh::edgeSplit(Halfedge *halfedge) {
    Halfedge *twin = halfedge->twin;

    // new vertex
    Vertex *new_vertex = new Vertex;
    new_vertex->pos = (halfedge->vertex->pos + twin->vertex->pos)/2;

    Vertex *upVertex = twin->vertex;
    Vertex *rightVertex = twin->next->next->vertex;
    Vertex *bottomVertex = halfedge->vertex;
    Vertex *leftVertex = halfedge->next->next->vertex;

    Face *topLeftFace = new Face;
    Face *topRightFace = new Face;
    Face *bottomLeftFace = halfedge->face;
    Face *bottomRightFace = twin->face;

    Edge *upEdge = new Edge;
    Halfedge *upHalfedge = new Halfedge;
    Halfedge *upTwin = new Halfedge;

    Edge *leftEdge = new Edge;
    Halfedge *leftHalfedge = new Halfedge;
    Halfedge *leftTwin = new Halfedge;

    Edge *rightEdge = new Edge;
    Halfedge *rightHalfedge = new Halfedge;
    Halfedge *rightTwin = new Halfedge;

    Edge *bottomEdge = twin->edge;
    Halfedge *bottomHalfedge = twin;
    Halfedge *bottomTwin = halfedge;

    Halfedge *topLeft = halfedge->next;
    Halfedge *bottomLeft = topLeft->next;
    Halfedge *bottomRight = twin->next;
    Halfedge *topRight = bottomRight->next;

    // set up the new vertex
    bottomHalfedge->vertex = new_vertex;
    upHalfedge->vertex = new_vertex;
    rightHalfedge->vertex = new_vertex;
    leftHalfedge->vertex = new_vertex;

    new_vertex->halfedge = upHalfedge;

    // set up the twins's vertices
    bottomTwin->vertex = bottomVertex;
    leftTwin->vertex = leftVertex;
    upTwin->vertex = upVertex;
    rightTwin->vertex = rightVertex;

    // set up the faces
    bottomHalfedge->face = bottomRightFace;
    upHalfedge->face = topLeftFace;
    rightHalfedge->face = topRightFace;
    leftHalfedge->face = bottomLeftFace;

    bottomTwin->face = bottomLeftFace;
    upTwin->face = topRightFace;
    rightTwin->face = bottomRightFace;
    leftTwin->face = topLeftFace;

    bottomLeftFace->halfedge = bottomTwin;
    bottomRightFace->halfedge = bottomHalfedge;
    topLeftFace->halfedge = upHalfedge;
    topRightFace->halfedge = upTwin;

    topLeft->face = topLeftFace;
    topRight->face = topRightFace;
    bottomLeft->face = bottomLeftFace;
    bottomRight->face = bottomRightFace;

    // set up the edges
    leftHalfedge->edge = leftEdge;
    leftTwin->edge = leftEdge;
    leftEdge->halfedge=leftHalfedge;

    rightHalfedge->edge = rightEdge;
    rightTwin->edge = rightEdge;
    rightEdge->halfedge=rightHalfedge;

    upHalfedge->edge = upEdge;
    upTwin->edge = upEdge;
    upEdge->halfedge=upHalfedge;

    bottomHalfedge->edge = bottomEdge;
    bottomTwin->edge = bottomEdge;
    bottomEdge->halfedge=bottomHalfedge;

    // set up twins
    leftHalfedge->twin = leftTwin;
    leftTwin->twin = leftHalfedge;

    rightHalfedge->twin = rightTwin;
    rightTwin->twin = rightHalfedge;

    upHalfedge->twin = upTwin;
    upTwin->twin = upHalfedge;

    bottomHalfedge->twin = bottomTwin;
    bottomTwin->twin = bottomHalfedge;

    // set up "next"
    leftHalfedge->next = bottomLeft;
    bottomLeft->next = bottomTwin;
    bottomTwin->next = leftHalfedge;

    leftTwin->next = upHalfedge;
    upHalfedge->next = topLeft;
    topLeft->next = leftTwin;

    rightHalfedge->next = topRight;
    topRight->next = upTwin;
    upTwin->next = rightHalfedge;

    rightTwin->next = bottomHalfedge;
    bottomHalfedge->next = bottomRight;
    bottomRight->next = rightTwin;

    // add new halfedges to mesh
    _halfedges[leftHalfedge] = leftHalfedge;
    _halfedges[leftTwin] = leftTwin;

    _halfedges[rightHalfedge] = rightHalfedge;
    _halfedges[rightTwin] = rightTwin;

    _halfedges[upHalfedge] = upHalfedge;
    _halfedges[upTwin] = upTwin;

    upEdge->is_new = false;
    bottomEdge->is_new = false;
    leftEdge->is_new = true;
    rightEdge->is_new = true;

    return new_vertex;
}
