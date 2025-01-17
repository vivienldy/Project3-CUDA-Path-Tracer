#include <iostream>
#include "scene.h"
#include <cstring>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>
#include "tiny_obj_loader.h"

Scene::Scene(string filename) {
    cout << "Reading scene from " << filename << " ..." << endl;
    cout << " " << endl;
    char* fname = (char*)filename.c_str();
    fp_in.open(fname);
    if (!fp_in.is_open()) {
        cout << "Error reading from file - aborting!" << endl;
        throw;
    }
    while (fp_in.good()) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            if (strcmp(tokens[0].c_str(), "MATERIAL") == 0) {
                loadMaterial(tokens[1]);
                cout << " " << endl;
            } else if (strcmp(tokens[0].c_str(), "OBJECT") == 0) {
                loadGeom(tokens[1]);
                cout << " " << endl;
            } else if (strcmp(tokens[0].c_str(), "CAMERA") == 0) {
                loadCamera();
                cout << " " << endl;
            }
        }
    }
}

int Scene::loadGeom(string objectid) {
    int id = atoi(objectid.c_str());
    if (id != geoms.size()) {
        cout << "ERROR: OBJECT ID does not match expected number of geoms" << endl;
        return -1;
    } else {
        cout << "Loading Geom " << id << "..." << endl;
        Geom newGeom;
        string line;

        //load object type
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            if (strcmp(line.c_str(), "sphere") == 0) {
                cout << "Creating new sphere..." << endl;
                newGeom.type = SPHERE;
            } else if (strcmp(line.c_str(), "cube") == 0) {
                cout << "Creating new cube..." << endl;
                newGeom.type = CUBE;
            } else if (strcmp(line.c_str(), "mesh") == 0) {
                cout << "Creating new mesh..." << endl;
                newGeom.type = MESH;
            }
        }

        //link material
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            newGeom.materialid = atoi(tokens[1].c_str());
            cout << "Connecting Geom " << objectid << " to Material " << newGeom.materialid << "..." << endl;

            // if the geom is light push it back to scene lights vector
            if (materials[newGeom.materialid].emittance > 0.f) {
                lights.push_back(newGeom);
            }
        }

        //load transformations
        //utilityCore::safeGetline(fp_in, line);
        //while (!line.empty() && fp_in.good()) {
        //    vector<string> tokens = utilityCore::tokenizeString(line);

        //    //load tranformations
        //    if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
        //        newGeom.translation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        //    } else if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
        //        newGeom.rotation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        //    } else if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
        //        newGeom.scale = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        //    }

        //    utilityCore::safeGetline(fp_in, line);
        //}
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            //load tranformations
            if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
                newGeom.translation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }
        }

        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            //load tranformations
            if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
            newGeom.rotation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }
        }

        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            //load tranformations
            if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
                newGeom.scale = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }
        }

        newGeom.transform = utilityCore::buildTransformationMatrix(
                newGeom.translation, newGeom.rotation, newGeom.scale);
        newGeom.inverseTransform = glm::inverse(newGeom.transform);
        newGeom.invTranspose = glm::inverseTranspose(newGeom.transform);

        // read filepath and load obj from file
        // after createing transform matrix 
        // because need transform matrix to transform postion to create AABB
        if (newGeom.type == MESH) {
            utilityCore::safeGetline(fp_in, line);
            if (!line.empty() && fp_in.good()) {
                vector<string> tokens = utilityCore::tokenizeString(line);
                const char* filePath = tokens[1].c_str();
                LoadMeshFromOBJ(filePath, newGeom);
                std::cout << "creating mesh from file: " << filePath << "..." << std::endl;
                hasMesh = true;
                meshGeomId = id;
            }
        }

        geoms.push_back(newGeom);
        return 1;
    }
}

int Scene::loadCamera() {
    cout << "Loading Camera ..." << endl;
    RenderState &state = this->state;
    Camera &camera = state.camera;
    float fovy;

    //load static properties
    for (int i = 0; i < 5; i++) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "RES") == 0) {
            camera.resolution.x = atoi(tokens[1].c_str());
            camera.resolution.y = atoi(tokens[2].c_str());
        } else if (strcmp(tokens[0].c_str(), "FOVY") == 0) {
            fovy = atof(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "ITERATIONS") == 0) {
            state.iterations = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "DEPTH") == 0) {
            state.traceDepth = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "FILE") == 0) {
            state.imageName = tokens[1];
        }
    }

    string line;
    utilityCore::safeGetline(fp_in, line);
    while (!line.empty() && fp_in.good()) {
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "EYE") == 0) {
            camera.position = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "LOOKAT") == 0) {
            camera.lookAt = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "UP") == 0) {
            camera.up = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        }

        utilityCore::safeGetline(fp_in, line);
    }

    //calculate fov based on resolution
    float yscaled = tan(fovy * (PI / 180));
    float xscaled = (yscaled * camera.resolution.x) / camera.resolution.y;
    float fovx = (atan(xscaled) * 180) / PI;
    camera.fov = glm::vec2(fovx, fovy);

    camera.right = glm::normalize(glm::cross(camera.view, camera.up));
    camera.pixelLength = glm::vec2(2 * xscaled / (float)camera.resolution.x,
                                   2 * yscaled / (float)camera.resolution.y);

    camera.view = glm::normalize(camera.lookAt - camera.position);

    //set up render camera stuff
    int arraylen = camera.resolution.x * camera.resolution.y;
    state.image.resize(arraylen);
    std::fill(state.image.begin(), state.image.end(), glm::vec3());

    cout << "Loaded camera!" << endl;
    return 1;
}

int Scene::loadMaterial(string materialid) {
    int id = atoi(materialid.c_str());
    if (id != materials.size()) {
        cout << "ERROR: MATERIAL ID does not match expected number of materials" << endl;
        return -1;
    } else {
        cout << "Loading Material " << id << "..." << endl;
        Material newMaterial;

        //load static properties
        for (int i = 0; i < 7; i++) {
            string line;
            utilityCore::safeGetline(fp_in, line);
            vector<string> tokens = utilityCore::tokenizeString(line);
            if (strcmp(tokens[0].c_str(), "RGB") == 0) {
                glm::vec3 color( atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()) );
                newMaterial.color = color;
            } else if (strcmp(tokens[0].c_str(), "SPECEX") == 0) {
                newMaterial.specular.exponent = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "SPECRGB") == 0) {
                glm::vec3 specColor(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
                newMaterial.specular.color = specColor;
            } else if (strcmp(tokens[0].c_str(), "REFL") == 0) {
                newMaterial.hasReflective = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFR") == 0) {
                newMaterial.hasRefractive = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFRIOR") == 0) {
                newMaterial.indexOfRefraction = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "EMITTANCE") == 0) {
                newMaterial.emittance = atof(tokens[1].c_str());
            }
        }
        materials.push_back(newMaterial);
        return 1;
    }
}

int LoadMeshFromOBJ(const char* filePath, Geom& mesh) {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<int> posIndex;
    std::vector<int> normIndex;
    string errors = tinyobj::SimpleLoadObj(positions, normals, posIndex, normIndex, filePath);
    std::cout << errors << std::endl;
    std::cout << positions[0] << std::endl;
    std::cout << normals[0] << std::endl;
    if (errors.size() == 0) {
        mesh.numTris = (int)(posIndex.size() / 3.f);
        mesh.triangles = new Triangle[mesh.numTris];
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 n0;
        glm::vec3 n1;
        glm::vec3 n2;
        int triCount = 0;
        for (int i = 0; i < posIndex.size(); i = i + 3)
        {
            Triangle triangle;
            int idx0 = posIndex[i] * 3;
            int idx1 = posIndex[i + 1] * 3;
            int idx2 = posIndex[i + 2] * 3;

            // every three float make a pos
            p0 = glm::vec3(positions[idx0], positions[idx0 + 1], positions[idx0 + 2]);
            p1 = glm::vec3(positions[idx1], positions[idx1 + 1], positions[idx1 + 2]);
            p2 = glm::vec3(positions[idx2], positions[idx2 + 1], positions[idx2 + 2]);
            
            // use three pos to create a triangle
            triangle.p0 = p0;
            triangle.p1 = p1;
            triangle.p2 = p2;

#if USE_BOUNDING_BOX
            glm::vec3 worldP0 = glm::vec3(mesh.transform *  glm::vec4(p0, 1.0f));
            glm::vec3 worldP1 = glm::vec3(mesh.transform * glm::vec4(p1, 1.0f));
            glm::vec3 worldP2 = glm::vec3(mesh.transform * glm::vec4(p2, 1.0f));
            
            mesh.AABBmax.x = max(mesh.AABBmax.x, worldP0.x);
            mesh.AABBmax.x = max(mesh.AABBmax.x, worldP1.x);
            mesh.AABBmax.x = max(mesh.AABBmax.x, worldP2.x);
            mesh.AABBmin.x = min(mesh.AABBmin.x, worldP0.x);
            mesh.AABBmin.x = min(mesh.AABBmin.x, worldP1.x);
            mesh.AABBmin.x = min(mesh.AABBmin.x, worldP2.x);
            
            mesh.AABBmax.y = max(mesh.AABBmax.y, worldP0.y);
            mesh.AABBmax.y = max(mesh.AABBmax.y, worldP1.y);
            mesh.AABBmax.y = max(mesh.AABBmax.y, worldP2.y);
            mesh.AABBmin.y = min(mesh.AABBmin.y, worldP0.y);
            mesh.AABBmin.y = min(mesh.AABBmin.y, worldP1.y);
            mesh.AABBmin.y = min(mesh.AABBmin.y, worldP2.y);
            
            mesh.AABBmax.z = max(mesh.AABBmax.z, worldP0.z);
            mesh.AABBmax.z = max(mesh.AABBmax.z, worldP1.z);
            mesh.AABBmax.z = max(mesh.AABBmax.z, worldP2.z);
            mesh.AABBmin.z = min(mesh.AABBmin.z, worldP0.z);
            mesh.AABBmin.z = min(mesh.AABBmin.z, worldP1.z);
            mesh.AABBmin.z = min(mesh.AABBmin.z, worldP2.z);
#endif
            // normal
            int nIdx0 = normIndex[i] * 3;
            int nIdx1 = normIndex[i + 1] * 3;
            int nIdx2 = normIndex[i + 2] * 3;

            n0 = glm::vec3(normals[nIdx0], normals[nIdx0 + 1], normals[nIdx0 + 2]);
            n1 = glm::vec3(normals[nIdx1], normals[nIdx1 + 1], normals[nIdx1 + 2]);
            n2 = glm::vec3(normals[nIdx2], normals[nIdx2 + 1], normals[nIdx2 + 2]);

            triangle.n0 = n0;
            triangle.n1 = n1;
            triangle.n2 = n2;

            mesh.triangles[triCount] = triangle;

            triCount++;
        }
    }
    return 1;
}
