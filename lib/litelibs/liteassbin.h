/*
LiteAssbin
Public domain Assimp binary model loader
It does not support animations
Can be used with LiteGFX
Created by Javier San Juan Cervera
No warranty implied. Use as you wish and at your own risk
*/

#ifndef LITE_ASSBIN_H
#define LITE_ASSBIN_H

#ifdef LITE_ASSBIN_USE_GFX
#include "litegfx.h"
#endif

#define LASSBIN_TEXTURE_DIFFUSE 0x1
#define LASSBIN_TEXTURE_SPECULAR 0x2
#define LASSBIN_TEXTURE_AMBIENT 0x3
#define LASSBIN_TEXTURE_EMISSIVE 0x4
#define LASSBIN_TEXTURE_HEIGHT 0x5
#define LASSBIN_TEXTURE_NORMALS 0x6
#define LASSBIN_TEXTURE_SHININESS 0x7
#define LASSBIN_TEXTURE_OPACITY 0x8
#define LASSBIN_TEXTURE_DISPLACEMENT 0x9
#define LASSBIN_TEXTURE_LIGHTMAP 0xA
#define LASSBIN_TEXTURE_REFLECTION 0xB

#define LASSBIN_MAX_COLOR_SETS 0x8
#define LASSBIN_MAX_TEXCOORD_SETS 0x8

#ifdef __cplusplus
extern "C" {
#endif

/* header */
typedef struct
{
  char magic_id[44]; /* ASSIMP.binary */
  int ver_major;
  int ver_minor;
  int revision;
  int flags;
  short dump_format;
  short compressed;
  char filename[256];
  char params[128];
  char reserved[64];
} lassbin_header_t;

/* chunk header */
typedef struct
{
  int magic_id; /* one of LASSBIN_CHUNK_* macros */
  int length;   /* length of the chunk (not counting this header) */
} lassbin_chunk_header_t;

/* node */
typedef struct
{
  char  name[128];
  float transform[16];
  int   num_children;
  int   num_meshes;
  int*  mesh_indices;
} lassbin_node_t;

/* face */
typedef struct
{
  unsigned short  num_indices;
  void* indices; /* unsigned short if num_indices <= 65536, otherwise unsigned int */
} lassbin_face_t;

/* mesh */
typedef struct
{
  int primitive_types;
  int num_vertices;
  int num_faces;
  int num_bones;
  int material_index;
  int components;
  float* positions;
  float* normals;
  float* tangents;
  float* bitangents;
  float* colors[LASSBIN_MAX_COLOR_SETS];
  int    numuvs[LASSBIN_MAX_TEXCOORD_SETS];
  float* texcoords[LASSBIN_MAX_TEXCOORD_SETS];
  lassbin_face_t* faces;
} lassbin_mesh_t;

/* material property */
typedef struct
{
  char key[128];
  int semantic;
  int index;
  int data_length;
  int type;
  char* data;
} lassbin_matproperty_t;

/* material */
typedef struct
{
  int                    num_properties;
  lassbin_matproperty_t* properties;
} lassbin_material_t;

/* texture */
typedef struct
{
  int            width;
  int            height;
  char           format_hint[4];
  unsigned char* data;
} lassbin_texture_t;

/* scene */
typedef struct
{
  int flags;
  int num_meshes;
  int num_materials;
  int num_animations;
  int num_textures;
  int num_lights;
  int num_cameras;
  lassbin_mesh_t* meshes;
  lassbin_material_t* materials;
  lassbin_texture_t* textures;
} lassbin_scene_t;

lassbin_scene_t* lassbin_load(const char* filename);
void lassbin_free(lassbin_scene_t* scene);
const char* lassbin_matname(const lassbin_material_t* material);
int lassbin_matnumtextures(const lassbin_material_t* material, int type);
const char* lassbin_mattexturename(const lassbin_material_t* material, int type, int index);
float lassbin_matopacity(const lassbin_material_t* material);
float* lassbin_matdiffuse(const lassbin_material_t* material);
float* lassbin_matemissive(const lassbin_material_t* material);
float* lassbin_matspecular(const lassbin_material_t* material);
float* lassbin_matambient(const lassbin_material_t* material);
float lassbin_matshininess(const lassbin_material_t* material);
float lassbin_matshinpercent(const lassbin_material_t* material);
/*const char* lassbin_matblend(const lassbin_material_t* material);*/
unsigned int lassbin_texturesize(const lassbin_texture_t* texture);

#ifdef LITE_ASSBIN_USE_GFX
lvert_t* lassbin_getvertices(const lassbin_mesh_t* mesh);
unsigned short* lassbin_getindices(const lassbin_mesh_t* mesh, int* num_indices);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LITE_ASSBIN_H */



/* IMPLEMENTATION */



#ifdef LITE_ASSBIN_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* tmagic chunk identifiers */
#define LASSBIN_CHUNK_AICAMERA                   0x1234
#define LASSBIN_CHUNK_AILIGHT                    0x1235
#define LASSBIN_CHUNK_AITEXTURE                  0x1236
#define LASSBIN_CHUNK_AIMESH                     0x1237
#define LASSBIN_CHUNK_AINODEANIM                 0x1238
#define LASSBIN_CHUNK_AISCENE                    0x1239
#define LASSBIN_CHUNK_AIBONE                     0x123a
#define LASSBIN_CHUNK_AIANIMATION                0x123b
#define LASSBIN_CHUNK_AINODE                     0x123c
#define LASSBIN_CHUNK_AIMATERIAL                 0x123d
#define LASSBIN_CHUNK_AIMATERIALPROPERTY         0x123e

#define LASSBIN_MESH_HAS_POSITIONS                   0x1
#define LASSBIN_MESH_HAS_NORMALS                     0x2
#define LASSBIN_MESH_HAS_TANGENTS_AND_BITANGENTS     0x4
#define LASSBIN_MESH_HAS_TEXCOORD_BASE               0x100
#define LASSBIN_MESH_HAS_COLOR_BASE                  0x10000

#define LASSBIN_MESH_HAS_TEXCOORD(n) (LASSBIN_MESH_HAS_TEXCOORD_BASE << n)
#define LASSBIN_MESH_HAS_COLOR(n)    (LASSBIN_MESH_HAS_COLOR_BASE << n)

#define LASSBIN_SCENE_FIXED_SIZE 28
#define LASSBIN_NODE_FIXED_SIZE 72
#define LASSBIN_MESH_FIXED_SIZE 24
#define LASSBIN_MATERIAL_FIXED_SIZE 4
#define LASSBIN_MATPROPERTY_FIXED_SIZE 16
#define LASSBIN_TEXTURE_FIXED_SIZE 12

#ifdef __cplusplus
extern "C" {
#endif

static lassbin_scene_t* _lassbin_load_scene(FILE* fp);
static int _lassbin_load_node(FILE* fp);
static int _lassbin_load_mesh(FILE* fp, lassbin_mesh_t* mesh);
static int _lassbin_load_material(FILE* fp, lassbin_material_t* material);
static int _lassbin_load_matproperty(FILE* fp, lassbin_matproperty_t* prop);
static int _lassbin_load_texture(FILE* fp, lassbin_texture_t* texture);
static void _lassbin_load_string(FILE* fp, char* out, int len);
static void _lassbin_skip(FILE* fp);

lassbin_scene_t* lassbin_load(const char* filename)
{
  FILE* fp;
  lassbin_header_t header;
  lassbin_scene_t* scene;

  /* open file for reading */
  fp = fopen(filename, "rb");
  if (!fp) return 0;

  /* read header and compare tag */
  fread(&header, 1, sizeof(header), fp);
  if (strncmp(header.magic_id, "ASSIMP.binary", 13) != 0 || header.dump_format != 0 || header.compressed != 0)
  {
    fclose(fp);
    return 0;
  }

  /* read scene */
  return _lassbin_load_scene(fp);
}

void lassbin_free(lassbin_scene_t* scene)
{
  int i, j;

  for (i = 0; i < scene->num_meshes; ++i)
  {
    free(scene->meshes[i].positions);
    free(scene->meshes[i].normals);
    free(scene->meshes[i].tangents);
    free(scene->meshes[i].bitangents);
    for (j = 0; j < LASSBIN_MAX_COLOR_SETS; ++j)
    {
      free(scene->meshes[i].colors[j]);
    }
    for (j = 0; j < LASSBIN_MAX_TEXCOORD_SETS; ++j)
    {
      free(scene->meshes[i].texcoords[j]);
    }
    free(scene->meshes[i].faces);
  }
  for (i = 0; i < scene->num_materials; ++i)
  {
    for (j = 0; j < scene->materials[i].num_properties; ++j)
    {
      free(scene->materials[i].properties[j].data);
    }
    free(scene->materials[i].properties);
  }
  for (i = 0; i < scene->num_textures; ++i)
  {
    free(scene->textures[i].data);
  }
  free(scene->meshes);
  free(scene->materials);
  free(scene->textures);
  free(scene);
}

const char* lassbin_matname(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "?mat.name") == 0) return &prop->data[4];
  }

  return NULL;
}

int lassbin_matnumtextures(const lassbin_material_t* material, int type)
{
  int i;
  int num = 0;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$tex.file") == 0 && prop->semantic == type) ++num;
  }

  return num;
}

const char* lassbin_mattexturename(const lassbin_material_t* material, int type, int index)
{
  int i;
  int num = 0;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$tex.file") == 0 && prop->semantic == type) {
      if (num == index) return &prop->data[4];
      else ++num;
    }
  }

  return NULL;
}

float lassbin_matopacity(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$mat.opacity") == 0)
    {
      return *((float*)prop->data);
    }
  }

  return 1;
}

float* lassbin_matdiffuse(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$clr.diffuse") == 0)
    {
      return (float*)prop->data;
    }
  }

  return NULL;
}

float* lassbin_matemissive(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$clr.emissive") == 0)
    {
      return (float*)prop->data;
    }
  }

  return NULL;
}

float* lassbin_matspecular(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$clr.specular") == 0)
    {
      return (float*)prop->data;
    }
  }

  return NULL;
}

float* lassbin_matambient(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$clr.ambient") == 0)
    {
      return (float*)prop->data;
    }
  }

  return NULL;
}

float lassbin_matshininess(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$mat.shininess") == 0)
    {
      return *((float*)prop->data);
    }
  }

  return 0;
}

float lassbin_matshinpercent(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$mat.shinpercent") == 0)
    {
      return *((float*)prop->data);
    }
  }

  return 1;
}

/*
const char* lassbin_matblend(const lassbin_material_t* material)
{
  int i;

  for (i = 0; i < material->num_properties; ++i)
  {
    const lassbin_matproperty_t* prop = &material->properties[i];
    if (strcmp(prop->key, "$mat.blend") == 0)
    {
      //return &prop->data[4];
    }
  }

  return NULL;
}
*/

unsigned int lassbin_texturesize(const lassbin_texture_t* texture)
{
  if (texture->height != 0) return texture->width * texture->height * 4;
  else return texture->width;
}

/*
static void _lassbin_printid(int magic_id)
{
  switch (magic_id)
  {
    case LASSBIN_CHUNK_AICAMERA:
      printf("camera\n");
      break;
    case LASSBIN_CHUNK_AILIGHT:
      printf("light\n");
      break;
    case LASSBIN_CHUNK_AITEXTURE:
      printf("texture\n");
      break;
    case LASSBIN_CHUNK_AIMESH:
      printf("mesh\n");
      break;
    case LASSBIN_CHUNK_AINODEANIM:
      printf("nodeanim\n");
      break;
    case LASSBIN_CHUNK_AISCENE:
      printf("scene\n");
      break;
    case LASSBIN_CHUNK_AIBONE:
      printf("bone\n");
      break;
    case LASSBIN_CHUNK_AIANIMATION:
      printf("animation\n");
      break;
    case LASSBIN_CHUNK_AINODE:
      printf("node\n");
      break;
    case LASSBIN_CHUNK_AIMATERIAL:
      printf("material\n");
      break;
    case LASSBIN_CHUNK_AIMATERIALPROPERTY:
      printf("materialproperty\n");
      break;
    default:
      printf("unknown\n");
  }
}
*/

static lassbin_scene_t* _lassbin_load_scene(FILE* fp)
{
  lassbin_chunk_header_t header;
  lassbin_scene_t* scene;
  int i;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AISCENE) return 0;

  /* read scene */
  scene = (lassbin_scene_t*)calloc(1, sizeof(lassbin_scene_t));
  fread(scene, 1, LASSBIN_SCENE_FIXED_SIZE, fp);

  /* skip root node */
  _lassbin_skip(fp);
  /*if (!_lassbin_load_node(fp))
  {
    lassbin_free(scene);
    return 0;
  }*/

  /* load meshes */
  scene->meshes = (lassbin_mesh_t*)calloc(scene->num_meshes, sizeof(lassbin_mesh_t));
  for (i = 0; i < scene->num_meshes; ++i)
  {
    if (!_lassbin_load_mesh(fp, &scene->meshes[i]))
    {
      lassbin_free(scene);
      return 0;
    }
  }

  /* load materials */
  scene->materials = (lassbin_material_t*)calloc(scene->num_materials, sizeof(lassbin_material_t));
  for (i = 0; i < scene->num_materials; ++i)
  {
    if (!_lassbin_load_material(fp, &scene->materials[i]))
    {
      lassbin_free(scene);
      return 0;
    }
  }

  /* skip animations */
  for (i = 0; i < scene->num_animations; ++i)
  {
    _lassbin_skip(fp);
  }

  /* load textures */
  scene->textures = (lassbin_texture_t*)calloc(scene->num_textures, sizeof(lassbin_texture_t));
  for (i = 0; i < scene->num_textures; ++i)
  {
    if (!_lassbin_load_texture(fp, &scene->textures[i]))
    {
      lassbin_free(scene);
      return 0;
    }
  }

  /* load lights */
  /* ... */

  /* load cameras */
  /* ... */

  return scene;
}

static int _lassbin_load_node(FILE* fp)
{
  lassbin_chunk_header_t header;
  lassbin_node_t node;
  long end;
  int i;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AINODE) return 0;

  /* get end position of chunk, to skip metadata if present at end */
  end = ftell(fp) + header.length;

  /* read node */
  memset(&node, 0, sizeof(lassbin_node_t));
  _lassbin_load_string(fp, node.name, sizeof(node.name));
  fread(&node.transform, 1, LASSBIN_NODE_FIXED_SIZE, fp);
  node.mesh_indices = (int*)malloc(node.num_meshes * sizeof(int));

  /* load mesh indices */
  fread(node.mesh_indices, node.num_meshes, sizeof(int), fp);

  /* load children */
  for (i = 0; i < node.num_children; ++i) _lassbin_load_node(fp);

  /* skip metadata (if exported with "export" instead of "dump") */
  fseek(fp, end, SEEK_SET);

  return 1;
}

static int _lassbin_load_mesh(FILE* fp, lassbin_mesh_t* mesh)
{
  lassbin_chunk_header_t header;
  int i;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AIMESH) return 0;

  /* read mesh */
  memset(mesh, 0, sizeof(lassbin_mesh_t));
  fread(mesh, 1, LASSBIN_MESH_FIXED_SIZE, fp);

  /* read positions */
  if (mesh->components & LASSBIN_MESH_HAS_POSITIONS)
  {
    mesh->positions = (float*)malloc(mesh->num_vertices * 3 * sizeof(float));
    fread(mesh->positions, mesh->num_vertices * 3, sizeof(float), fp);
  }

  /* read normals */
  if (mesh->components & LASSBIN_MESH_HAS_POSITIONS)
  {
    mesh->normals = (float*)malloc(mesh->num_vertices * 3 * sizeof(float));
    fread(mesh->normals, mesh->num_vertices * 3, sizeof(float), fp);
  }

  /* read tangents and bitangents */
  if (mesh->components & LASSBIN_MESH_HAS_TANGENTS_AND_BITANGENTS)
  {
    mesh->tangents = (float*)malloc(mesh->num_vertices * 3 * sizeof(float));
    mesh->bitangents = (float*)malloc(mesh->num_vertices * 3 * sizeof(float));
    fread(mesh->tangents, mesh->num_vertices * 3, sizeof(float), fp);
    fread(mesh->bitangents, mesh->num_vertices * 3, sizeof(float), fp);
  }

  /* read color sets */
  for (i = 0; i < LASSBIN_MAX_COLOR_SETS; ++i)
  {
    if (mesh->components & LASSBIN_MESH_HAS_COLOR(i))
    {
      mesh->colors[i] = (float*)malloc(mesh->num_vertices * 4 * sizeof(float));
      fread(mesh->colors[i], mesh->num_vertices * 4, sizeof(float), fp);
    }
  }

  /* read tex coords */
  for (i = 0; i < LASSBIN_MAX_TEXCOORD_SETS; ++i)
  {
    if (mesh->components & LASSBIN_MESH_HAS_TEXCOORD(i))
    {
      mesh->texcoords[i] = (float*)malloc(mesh->num_vertices * 4 * sizeof(float));
      fread(&mesh->numuvs[i], 1, sizeof(int), fp);
      fread(mesh->texcoords[i], mesh->num_vertices * 3, sizeof(float), fp);
    }
  }

  /* read faces */
  mesh->faces = (lassbin_face_t*)malloc(mesh->num_faces * sizeof(lassbin_face_t));
  for (i = 0; i < mesh->num_faces; ++i)
  {
    int index_size = (mesh->num_vertices < (1u<<16)) ? sizeof(unsigned short) : sizeof(unsigned int);
    fread(&mesh->faces[i].num_indices, 1, sizeof(unsigned short), fp);
    mesh->faces[i].indices = malloc(mesh->faces[i].num_indices * index_size);
    fread(mesh->faces[i].indices, mesh->faces[i].num_indices, index_size, fp);
  }

  /* skip bones (unsupported yet) */
  for (i = 0; i < mesh->num_bones; ++i)
  {
    _lassbin_skip(fp);
  }

  return 1;
}

static int _lassbin_load_material(FILE* fp, lassbin_material_t* material)
{
  lassbin_chunk_header_t header;
  int i;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AIMATERIAL) return 0;

  /* read material */
  memset(material, 0, sizeof(lassbin_material_t));
  fread(material, 1, LASSBIN_MATERIAL_FIXED_SIZE, fp);

  /* read properties */
  material->properties = (lassbin_matproperty_t*)calloc(material->num_properties, sizeof(lassbin_matproperty_t));
  for (i = 0; i < material->num_properties; ++i)
  {
    if (!_lassbin_load_matproperty(fp, &material->properties[i]))
    {
      return 0;
    }
  }

  return 1;
}

static int _lassbin_load_matproperty(FILE* fp, lassbin_matproperty_t* prop)
{
  lassbin_chunk_header_t header;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AIMATERIALPROPERTY) return 0;

  /* read data */
  _lassbin_load_string(fp, prop->key, sizeof(prop->key));
  fread(&prop->semantic, 1, LASSBIN_MATPROPERTY_FIXED_SIZE, fp);
  prop->data = (char*)malloc(prop->data_length);
  fread(prop->data, 1, prop->data_length, fp);

  return 1;
}

static int _lassbin_load_texture(FILE* fp, lassbin_texture_t* texture)
{
  lassbin_chunk_header_t header;
  int i;

  /* read header */
  fread(&header, 1, sizeof(header), fp);
  if (header.magic_id != LASSBIN_CHUNK_AITEXTURE) return 0;

  /* read texture */
  memset(texture, 0, sizeof(lassbin_texture_t));
  fread(texture, 1, LASSBIN_TEXTURE_FIXED_SIZE, fp);

  /* read data */
  texture->data = (unsigned char*)malloc(lassbin_texturesize(texture));
  fread(texture->data, lassbin_texturesize(texture), sizeof(unsigned char), fp);

  return 1;
}



static void _lassbin_load_string(FILE* fp, char* out, int len)
{
  char* str;
  int length;
  fread(&length, 1, sizeof(length), fp);
  if (length > 0)
  {
    str = (char*)malloc(length);
    fread(str, 1, length, fp);
    strncpy(out, str, length < len ? length : len);
    out[len-1] = 0;
    free(str);
  }
  else
  {
    out[0] = 0;
  }
}

static void _lassbin_skip(FILE* fp)
{
  lassbin_chunk_header_t header;
  fread(&header, 1, sizeof(header), fp);
  fseek(fp, header.length, SEEK_CUR);
}

#ifdef LITE_ASSBIN_USE_GFX

lvert_t* lassbin_getvertices(const lassbin_mesh_t* mesh)
{
  lvert_t* verts;
  int v, i;

  /* create buffer */
  verts = (lvert_t*)malloc(mesh->num_vertices * sizeof(lvert_t));

  for (v = 0; v < mesh->num_vertices; ++v)
  {
    float x = 0, y = 0, z = 0;
    float nx = 0, ny = 0, nz = -1;
    float tu = 0, tv = 0;
    float r = 1, g = 1, b = 1, a = 1;

    if (mesh->components & LASSBIN_MESH_HAS_POSITIONS)
    {
      x = mesh->positions[v*3];
      y = mesh->positions[v*3+1];
      z = mesh->positions[v*3+2];
    }

    if (mesh->components & LASSBIN_MESH_HAS_NORMALS)
    {
      nx = mesh->normals[v*3];
      ny = mesh->normals[v*3+1];
      nz = mesh->normals[v*3+2];
    }

    for (i = 0; i < LASSBIN_MAX_COLOR_SETS; ++i)
    {
      if (mesh->components & LASSBIN_MESH_HAS_COLOR(i))
      {
        r = mesh->colors[i][v*4];
        g = mesh->colors[i][v*4+1];
        b = mesh->colors[i][v*4+2];
        a = mesh->colors[i][v*4+3];
        break;
      }
    }

    for (i = 0; i < LASSBIN_MAX_TEXCOORD_SETS; ++i)
    {
      if (mesh->components & LASSBIN_MESH_HAS_TEXCOORD(i))
      {
        tu = mesh->texcoords[i][v*3];
        tv = mesh->texcoords[i][v*3+1];
        break;
      }
    }

    verts[v] = lvert(x, y, z, nx, ny, nz, tu, tv, r, g, b, a);
  }

  return verts;
}

unsigned short* lassbin_getindices(const lassbin_mesh_t* mesh, int* num_indices)
{
  unsigned short* indices;
  int index_size;
  int f, i;

  /* get total number of indices */
  *num_indices = 0;
  for (f = 0; f < mesh->num_faces; ++f)
  {
    *num_indices += mesh->faces[f].num_indices;
  }

  /* if number of vertices >= 65536, can't load the indices */
  if (mesh->num_vertices >= (1u<<16)) return 0;

  /* create buffer */
  indices = (unsigned short*)malloc(*num_indices * sizeof(unsigned short));

  /* add indices */
  i = 0;
  for (f = 0; f < mesh->num_faces; ++f)
  {
    int i2;
    for (i2 = 0; i2 < mesh->faces[f].num_indices; ++i2)
    {
      indices[i] = ((unsigned short*)mesh->faces[f].indices)[i2];
      ++i;
    }
  }

  return indices;
}

#endif /* LITE_ASSBIN_USE_GFX */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LITE_ASSBIN_IMPLEMENTATION */
