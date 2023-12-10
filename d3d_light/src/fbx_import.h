#ifndef _FBX_IMPORT_H_
#define _FBX_IMPORT_H_
#include "stdafx.h"
#include "mesh_common.h"
#include <fbxsdk.h>

struct Fbx_Data {
    Mesh_Object *objects;
    uint num_objects;
};

struct Fbx_State {
    FbxManager *manager;
    FbxImporter *importer; // @TODO: Importer 'reinitialization' when loading multiple meshes?

    ~Fbx_State() {}
};
static Fbx_State fbx;

void initialize_fbx_sdk() {
    fbx.manager = FbxManager::Create();
    fbx.importer = FbxImporter::Create(fbx.manager, "");
    
    FbxIOSettings *io_settings = FbxIOSettings::Create(fbx.manager, IOSROOT);
    fbx.manager->SetIOSettings(io_settings);
}

void release_fbx_sdk() {
    fbx.importer->Destroy();
    fbx.manager->Destroy();
}

void fbx_get_mesh_count(FbxNode *node, Fbx_Data *data) {
    for (auto i = 0; i != node->GetNodeAttributeCount(); ++i) {
        if (node->GetNodeAttributeByIndex(i)->GetAttributeType() == FbxNodeAttribute::eMesh)
            data->num_objects++;
    }

    for (auto i = 0; i != node->GetChildCount(); ++i) {
        fbx_get_mesh_count(node->GetChild(i), data);
    }
}

void fbx_iter_mesh_data(FbxNode *node, Fbx_Data *data, bool is_root = false, int current_mesh_index = 0, uint parent_index = (uint)-1) {

    if (!is_root) {
        FbxNodeAttribute *mesh_attribute = NULL;
        for (auto i = 0; i != node->GetNodeAttributeCount(); ++i) {
            if (node->GetNodeAttributeByIndex(i)->GetAttributeType() == FbxNodeAttribute::eMesh) {
                mesh_attribute = node->GetNodeAttributeByIndex(i);
                break;
            }
        }

        if (mesh_attribute)
        {
            printf("Mesh: %s\n", mesh_attribute->GetName());
        
            // Retrieve basic data.
            FbxDouble3 node_position = node->LclTranslation.Get();
            FbxDouble3 node_rotation = node->LclRotation.Get();
            FbxDouble3 node_scaling = node->LclScaling.Get();

            data->objects[current_mesh_index].tf.position = HMM_V3(node_position[0], node_position[1], node_position[2]);
            data->objects[current_mesh_index].tf.rotation = HMM_V3(node_rotation[0], node_rotation[1], node_rotation[2]);
            data->objects[current_mesh_index].tf.scaling = HMM_V3(node_scaling[0] / 100, node_scaling[1] / 100, node_scaling[2] / 100);

            const char *node_name = node->GetName();
            memset(data->objects[current_mesh_index].name, 0, 1024);
            memcpy(data->objects[current_mesh_index].name, node_name, strlen(node_name));

            data->objects[current_mesh_index].parent_index = parent_index;

            FbxMesh *mesh = (FbxMesh *)mesh_attribute;

            // @TODO
            // @TODO: CONTINUE HERE SOMEWHERE
            // @TODO
            // triangulate mesh.
            FbxGeometryConverter fbx_geom_converter(fbx.manager);
            mesh = (FbxMesh *)fbx_geom_converter.Triangulate(mesh_attribute, true);

            // ensure usable normals.
            mesh->GenerateNormals(true, true);

            // Retrieve control points and normals.            
            int num_control_points = mesh->GetControlPointsCount();
            FbxVector4 *control_points = mesh->GetControlPoints();
            int num_polygons = mesh->GetPolygonCount();

            data->objects[current_mesh_index].num_vertices = num_control_points;
            data->objects[current_mesh_index].num_faces = num_polygons;
            
            printf("Vertex Count: %d\n",
                   num_control_points);

            for (auto i = 0; i != num_control_points; ++i) {
                // Normals.
                FbxGeometryElementNormal *normals = mesh->GetElementNormal(0);

                if (normals->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
                    auto direct_normals_array = normals->GetDirectArray().GetAt(i);

                    //printf("\t(%f, %f, %f), (%f, %f, %f)\n",
                    //       control_points[i][0], control_points[i][1], control_points[i][2],
                    //       direct_normals_array[0], direct_normals_array[1], direct_normals_array[2]);
                }
            }
        }
    }
    
    for (auto i = 0; i != node->GetChildCount(); ++i) {
        fbx_iter_mesh_data(node->GetChild(i), data, false, current_mesh_index++, current_mesh_index);
    }
}

bool fbx_populate_mesh_data(FbxNode *node, Fbx_Data *data) {
    if (!data->num_objects)
        return false;

    data->objects = (Mesh_Object *)malloc(data->num_objects * sizeof(Mesh_Object));

    // first node.
    fbx_iter_mesh_data(node, data, true);

    // child nodes.
    for (auto i = 0; i != node->GetChildCount(); ++i) {
        fbx_iter_mesh_data(node->GetChild(i), data);
    }

    return true;
}

bool import_fbx(char *path, Fbx_Data *out_data)
{ 
    if (!fbx.importer->Initialize(path, -1, fbx.manager->GetIOSettings())) {
        LOG("Call to FBxImporter::Initialize() failed.\n");
        LOGF("%s\n", fbx.importer->GetStatus().GetErrorString());
        return false;
    }

    FbxScene *fbx_scene = FbxScene::Create(fbx.manager, "");
    fbx.importer->Import(fbx_scene);

    ///
    FbxNode *root_node = fbx_scene->GetRootNode();
    
    // Get the number of meshes.
    fbx_get_mesh_count(root_node, out_data);
    fbx_populate_mesh_data(root_node, out_data);

    LOGF("Found '%d' meshes.\n", out_data->num_objects);
    for (auto i = 0; i != out_data->num_objects; ++i) {
        LOGF("\t%s: %d, (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n",
             out_data->objects[i].name, out_data->objects[i].parent_index,
             out_data->objects[i].tf.position[0], out_data->objects[i].tf.position[1], out_data->objects[i].tf.position[2],
             out_data->objects[i].tf.rotation[0], out_data->objects[i].tf.rotation[1], out_data->objects[i].tf.rotation[2],
             out_data->objects[i].tf.scaling[0], out_data->objects[i].tf.scaling[1], out_data->objects[i].tf.scaling[2]);
    }
    
    
    fbx.manager->Destroy();
    //free(file_buffer);
    return true;
}

#endif
