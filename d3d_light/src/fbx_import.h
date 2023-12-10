#ifndef _FBX_IMPORT_H_
#define _FBX_IMPORT_H_
#include "stdafx.h"
#include <fbxsdk.h>

__forceinline void print_tabs(int num) { for (auto i = 0; i != num; ++i) { printf("\t"); } }

FbxString get_fbx_attribute_type_name(FbxNodeAttribute::EType type) {
    switch (type) {
        case FbxNodeAttribute::eUnknown: return "unknown"; break;
        case FbxNodeAttribute::eNull: return "null"; break;
        case FbxNodeAttribute::eMarker: return "marker"; break;
        case FbxNodeAttribute::eSkeleton: return "skeleton"; break;
        case FbxNodeAttribute::eMesh: return "mesh"; break;
        case FbxNodeAttribute::eNurbs: return "nurbs"; break;
        case FbxNodeAttribute::ePatch: return "patch"; break;
        case FbxNodeAttribute::eCamera: return "camera"; break;
        case FbxNodeAttribute::eCameraStereo: return "stereo"; break;
        case FbxNodeAttribute::eCameraSwitcher: return "camera switcher"; break;
        case FbxNodeAttribute::eLight: return "light"; break;
        case FbxNodeAttribute::eOpticalReference: return "optical reference"; break;
        case FbxNodeAttribute::eOpticalMarker: return "marker"; break;
        case FbxNodeAttribute::eNurbsCurve: return "nurbs curve"; break;
        case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface"; break;
        case FbxNodeAttribute::eBoundary: return "boundary"; break;
        case FbxNodeAttribute::eNurbsSurface: return "nurbs surface"; break;
        case FbxNodeAttribute::eShape: return "shape"; break;
        case FbxNodeAttribute::eLODGroup: return "lod group"; break;
        case FbxNodeAttribute::eSubDiv: return "subdiv"; break;
        default: return "unknown2"; break;
    }
}

void print_fbx_attribute(FbxNodeAttribute *attrib, int num_tabs) {
    if (!attrib)
        return;

    FbxString type_name = get_fbx_attribute_type_name(attrib->GetAttributeType());
    FbxString attrib_name = attrib->GetName();
    print_tabs(num_tabs);
    printf("<attribute type='%s' name='%s'/>\n", type_name.Buffer(), attrib_name.Buffer());
}

void print_fbx_node(FbxNode *node, int num_tabs) {
    print_tabs(num_tabs);
    const char *node_name = node->GetName();
    FbxDouble3 translation = node->LclTranslation.Get();
    FbxDouble3 rotation = node->LclRotation.Get();
    FbxDouble3 scaling = node->LclScaling.Get();

    printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
           node_name,
           translation[0], translation[1], translation[2],
           rotation[0], rotation[1], rotation[2],
           scaling[0], scaling[1], scaling[2]);

    // print attributes.
    for (auto i = 0; i < node->GetNodeAttributeCount(); ++i)
        print_fbx_attribute(node->GetNodeAttributeByIndex(i), num_tabs + 1);

    // print children
    for (auto i = 0; i != node->GetChildCount(); ++i)
        print_fbx_node(node->GetChild(i), num_tabs + 1);

    print_tabs(num_tabs);
    printf("</node>\n");
}

#include "transform.h"

struct Mesh_Object {
    char name[1024];
    Transform tf;
    uint parent_index;
};

struct Fbx_Data {
    Mesh_Object *objects;
    uint num_objects;
};

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
            // Retrieve data.
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

bool import_fbx(char *path)
{
#if 0
    LARGE_INTEGER file_size;
    char *file_buffer = NULL;
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 128, NULL);

    ASSERT(file != (HANDLE)-1);
    GetFileSizeEx(file, &file_size);
    file_buffer = (char *)malloc(file_size.QuadPart + 1);
    file_buffer[file_size.QuadPart] = 0;
    ReadFile(file, buffer, file_size.QuadPart, NULL, NULL);
    CloseHandle(file);
#endif
    
    ///
    FbxManager *fbx_manager = FbxManager::Create();
    FbxIOSettings *io_settings = FbxIOSettings::Create(fbx_manager, IOSROOT);
    fbx_manager->SetIOSettings(io_settings);

    FbxImporter *fbx_importer = FbxImporter::Create(fbx_manager, "");
    if (!fbx_importer->Initialize(path, -1, fbx_manager->GetIOSettings())) {
        LOG("Call to FBxImporter::Initialize() failed.\n");
        LOGF("%s\n", fbx_importer->GetStatus().GetErrorString());
        return false;
    }

    FbxScene *fbx_scene = FbxScene::Create(fbx_manager, "");
    fbx_importer->Import(fbx_scene);
    fbx_importer->Destroy();

    ///
    FbxNode *root_node = fbx_scene->GetRootNode();
    Fbx_Data fbx_data = {};
    
    // Get the number of meshes.
    fbx_get_mesh_count(root_node, &fbx_data);
    fbx_populate_mesh_data(root_node, &fbx_data);

    LOGF("Found '%d' meshes.\n", fbx_data.num_objects);
    for (auto i = 0; i != fbx_data.num_objects; ++i) {
        LOGF("\t%s: %d, (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n",
             fbx_data.objects[i].name, fbx_data.objects[i].parent_index,
             fbx_data.objects[i].tf.position[0], fbx_data.objects[i].tf.position[1], fbx_data.objects[i].tf.position[2],
             fbx_data.objects[i].tf.rotation[0], fbx_data.objects[i].tf.rotation[1], fbx_data.objects[i].tf.rotation[2],
             fbx_data.objects[i].tf.scaling[0], fbx_data.objects[i].tf.scaling[1], fbx_data.objects[i].tf.scaling[2]);
    }
    
    // @TODO: REMOVE THIS WHEN DONE DEBUGGING
    free(fbx_data.objects);
    // @TODO
    
    fbx_manager->Destroy();
    //free(file_buffer);
    return true;
}

#endif
