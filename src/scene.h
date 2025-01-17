#pragma once

#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include "glm/glm.hpp"
#include "utilities.h"
#include "sceneStructs.h"

using namespace std;

class Scene {
private:
    ifstream fp_in;
    int loadMaterial(string materialid);
    int loadGeom(string objectid);
    int loadCamera();
public:
    Scene(string filename);
    ~Scene();

    std::vector<Geom> geoms;
    std::vector<Material> materials;
    std::vector<Geom> lights;
    RenderState state;
    // for mesh geom allocate buffer on GPU use
    bool hasMesh = false;
    int meshGeomId = -1;
};

int LoadMeshFromOBJ(const char* filePath, Geom& mesh);
