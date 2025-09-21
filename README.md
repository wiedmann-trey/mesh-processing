## Mesh Processing
This project implements various operations on meshes. It was completed for CS2240: Advanced Computer Graphics, at Brown University

### Features
- Half-edge mesh representation
- Loop subdivision
- [Quadric error simplification](https://www.cs.cmu.edu/~garland/Papers/quadrics.pdf)
- [Isotropic remeshing](https://www.graphics.rwth-aachen.de/media/papers/remeshing1.pdf) (this implementation may be bugged)

A cow mesh before and after simplification:
<img width="747" height="611" alt="cow_before" src="https://github.com/user-attachments/assets/886d1362-83c0-400d-b7c6-8830fe9f9ace" />

![](student_outputs/final/simplify_cow.png)

### Building and running the project
Make sure to run "git submodule update --init" after cloning. The project can be built in Qt Creator. Open the project via the CMakeList and set the working directory to the base directory of the project. Pass the path to the config file as a command line argument. See example config files in "template_inis/".
