## Mesh (milestone submission)

Please fill this out and submit your work to Gradescope by the milestone deadline.

### Mesh Validator

- Tests 0-4 : half edges have all fields
- Test 5 : each half edge has two edges
- Test 6 : we can follow halfedges in a loop around a face back to the original one
- Test 7 : halfedges are twins of each other
- Test 8 : follow the halfedges around a vertex, they should share that vertex
- Test 9 : we have a disc around a vertex
- Test 10 : halfedges of same face share that face

### Implementation Locations

Please link to the lines (in GitHub) where the implementations of these features start:

- [Mesh data structure](https://github.com/brown-cs-224/mesh-wiedmann-trey/blob/314a742ea11919370f3ebbfac1212e37a73ad79f/mesh.h#L36)
- [Mesh validator](https://github.com/brown-cs-224/mesh-wiedmann-trey/blob/314a742ea11919370f3ebbfac1212e37a73ad79f/mesh.cpp#L173)

### Collaboration/References
Worked alone

### Known Bugs
Crashing right now when trying to build a mesh
