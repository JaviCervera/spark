/*
LiteGfx
Public domain graphics library based on OpenGL
It is intended to be used as a graphics library for simple applications
that do not require shaders or any modern graphics techniques
(it uses the fixed pipeline)
Should work on ancient GPUs
Created by Javier San Juan Cervera
No warranty implied. Use as you wish and at your own risk
*/

#ifndef LITE_GFX_H
#define LITE_GFX_H

#ifdef __cplusplus
extern "C"
{
#endif

/* main */

typedef enum
{
  B_SOLID,
  B_ALPHA,
  B_ADD,
  B_MUL
} lblend_t;

void lgfx_init();
void lgfx_setup2d(int width, int height);
void lgfx_setup3d(int width, int height);
void lgfx_setviewport(int x, int y, int width, int height);
void lgfx_setorigin(float x, float y);
void lgfx_setresolution(int width, int height);
void lgfx_setprojection(const float* m);
void lgfx_setmodelview(const float* m);
void lgfx_setblend(lblend_t mode);
void lgfx_setcolor(float r, float g, float b, float a);
void lgfx_setemissive(float r, float g, float b);
void lgfx_setspecular(float r, float g, float b);
void lgfx_setshininess(unsigned char shininess);
void lgfx_setusevertexcolor(int enable);
void lgfx_setculling(int enable);
void lgfx_setdepthwrite(int enable);
void lgfx_setlighting(int numlights);
void lgfx_setlight(int num, float x, float y, float z, float w, float r, float g, float b, int color_specular, float att); /* position must be passed in viewer space */
void lgfx_setambient(float r, float g, float b);
void lgfx_setfog(int enable, float r, float g, float b, float start, float end);
void lgfx_clearcolorbuffer(float r, float g, float b);
void lgfx_cleardepthbuffer();
void lgfx_drawpoint(float x, float y);
void lgfx_drawline(float x0, float y0, float x1, float y1);
void lgfx_drawrect(float x, float y, float width, float height);
void lgfx_drawoval(float x, float y, float width, float height);
int lgfx_multitexture_supported();
int lgfx_mipmapping_supported();

/* texture */

typedef enum
{
  F_NONE,
  F_LINEAR,
  F_MIPMAP
} ltexfilter_t;

typedef struct
{
  int glid;
  int width;
  int height;
  int filter;
} ltex_t;

ltex_t* ltex_alloc(int width, int height, int filter);
void ltex_free(ltex_t* tex);
void ltex_setpixels(const ltex_t* tex, const unsigned char* pixels);
void ltex_getpixels(const ltex_t* tex, unsigned char* out_pixels);
void ltex_draw(const ltex_t* tex, float x, float y);
void ltex_drawrot(const ltex_t* tex, float x, float y, float angle, float pivotx, float pivoty);
void ltex_drawrotsized(const ltex_t* tex, float x, float y, float angle, float pivotx, float pivoty, float width, float height, float u0, float v0, float u1, float v1);
void ltex_bind(const ltex_t* tex, const ltex_t* lightmap, int use_envlights);

/* vertex */

typedef enum
{
  R_POINTS,
  R_LINES,
  R_TRIANGLES,
  R_TRIANGLE_STRIP,
  R_TRIANGLE_FAN
} lrendermode_t;

typedef struct
{
  float pos[3];
  float nor[3];
  float tex[2];
  float tex2[2];
  float col[4];
} lvert_t;

lvert_t lvert(float x, float y, float z, float nx, float ny, float nz, float u, float v, float r, float g, float b, float a);
void lvert_draw(const lvert_t* vertices, unsigned int count, lrendermode_t mode);
void lvert_drawindexed(const lvert_t* vertices, const unsigned short* indices, unsigned int count, lrendermode_t mode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LITE_GFX_H */




/* IMPLEMENTATION */




#ifdef LITE_GFX_IMPLEMENTATION

/* include opengl */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include <GL/gl.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define OVALPOINTS 64

#ifdef __cplusplus
extern "C"
{
#endif

/* these are defined here because they are not part of opengl 1.1 (the one included by default on windows) */
#ifdef _WIN32
static PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
static PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture = NULL;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
#endif

/* setup */

void lgfx_init()
{
  /* get extension functions in windows */
#ifdef _WIN32
  glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
  glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
  glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
#endif
}

void lgfx_setup2d(int width, int height)
{
  glDisable(GL_ALPHA_TEST);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  if (lgfx_multitexture_supported())
  {
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE1);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);
  }
  lgfx_setviewport(0, 0, width, height);
  lgfx_setorigin(0, 0);
  lgfx_setresolution(width, height);
  lgfx_setblend(B_ALPHA);
  lgfx_setcolor(1, 1, 1, 1);
  lgfx_setusevertexcolor(0);
  lgfx_setculling(0);
  lgfx_setlighting(0);
  lgfx_setfog(0,0,0,0,0,0);
  ltex_bind(0, 0, 0);
}

void lgfx_setup3d(int width, int height)
{
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glAlphaFunc(GL_GEQUAL, 0.0001f);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glDepthFunc(GL_LEQUAL);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFrontFace(GL_CW);
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
  if (lgfx_multitexture_supported())
  {
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE1);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);
  }
  lgfx_setviewport(0, 0, width, height);
  lgfx_setorigin(0, 0);
  lgfx_setblend(B_SOLID);
  lgfx_setcolor(1, 1, 1, 1);
  lgfx_setusevertexcolor(0);
  lgfx_setculling(1);
  lgfx_setdepthwrite(1);
  lgfx_setlighting(0);
  lgfx_setfog(0,0,0,0,0,0);
  ltex_bind(0, 0, 0);
}

void lgfx_setviewport(int x, int y, int width, int height)
{
  glViewport(x, y, width, height);
  glScissor(x, y, width, height);
  lgfx_setresolution(width, height);
}

void lgfx_setorigin(float x, float y)
{
  glLoadIdentity();
  glTranslatef(-x, -y, 0);
}

void lgfx_setresolution(int width, int height)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, 0, 1000);
  glMatrixMode(GL_MODELVIEW);
}

void lgfx_setprojection(const float* m)
{
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m);
  glMatrixMode(GL_MODELVIEW);
}

void lgfx_setmodelview(const float* m)
{
  glLoadMatrixf(m);
}

void lgfx_setblend(lblend_t mode)
{
  switch (mode)
  {
  case B_SOLID:
    glBlendFunc(GL_ONE, GL_ZERO);
    break;
  case B_ALPHA:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case B_ADD:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    break;
  case B_MUL:
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    break;
  }
}

void lgfx_setcolor(float r, float g, float b, float a)
{
  glColor4f(r, g, b, a);
  // could be glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &color);
}

void lgfx_setemissive(float r, float g, float b)
{
  float emissive[4] = {r, g, b, 1};
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive);
}

void lgfx_setspecular(float r, float g, float b)
{
  float specular[4] = {r, g, b, 1};
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
}

void lgfx_setshininess(unsigned char shininess)
{
  glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void lgfx_setusevertexcolor(int enable)
{
  if (enable)
  {
    glEnableClientState(GL_COLOR_ARRAY);
  } else {
    glDisableClientState(GL_COLOR_ARRAY);
  }
}

void lgfx_setculling(int enable)
{
  if (enable)
  {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
}

void lgfx_setdepthwrite(int enable)
{
  glDepthMask((GLboolean)enable);
}

void lgfx_setlighting(int numlights)
{
  int i;

  if (numlights > 0) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
  for (i = 0; i < numlights; ++i) glEnable(GL_LIGHT0+i);
  for (i = numlights; i < 8; ++i) glDisable(GL_LIGHT0+i);
}

void lgfx_setlight(int num, float x, float y, float z, float w, float r, float g, float b, int color_specular, float att)
{
  float pos[4];
  float col[4];

  pos[0] = x;
  pos[1] = y;
  pos[2] = z;
  pos[3] = w;
  col[0] = r;
  col[1] = g;
  col[2] = b;
  col[3] = 1;
  glLightfv(GL_LIGHT0 + num, GL_POSITION, pos);
  glLightfv(GL_LIGHT0 + num, GL_DIFFUSE, col);
  if (color_specular == 1) {
    glLightfv(GL_LIGHT0 + num, GL_SPECULAR, col);
  } else {
    float white[4] = {1, 1, 1, 1};
    glLightfv(GL_LIGHT0 + num, GL_SPECULAR, white);
  }
  glLightf(GL_LIGHT0 + num, GL_LINEAR_ATTENUATION, att);
}

void lgfx_setambient(float r, float g, float b)
{
  float ambient[4];
  
  ambient[0] = r;
  ambient[1] = g;
  ambient[2] = b;
  ambient[3] = 1;
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
}

void lgfx_setfog(int enable, float r, float g, float b, float start, float end)
{
  float color[4];

  color[0] = r;
  color[1] = g;
  color[2] = b;
  color[3] = 1;
  if (enable)
  {
    glEnable(GL_FOG);
  } else {
    glDisable(GL_FOG);
  }
  glFogfv(GL_FOG_COLOR, color);
  glFogf(GL_FOG_START, start);
  glFogf(GL_FOG_END, end);
}

void lgfx_clearcolorbuffer(float r, float g, float b)
{
  glClearColor(r, g, b, 1);
  glClear(GL_COLOR_BUFFER_BIT);
}

void lgfx_cleardepthbuffer()
{
  glClear(GL_DEPTH_BUFFER_BIT);
}

void lgfx_drawpoint(float x, float y)
{
  lvert_t vert;
  
  vert = lvert(x, y, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  ltex_bind(0, 0, 0);
  lvert_draw(&vert, 1, R_POINTS);
}

void lgfx_drawline(float x0, float y0, float x1, float y1)
{
  lvert_t verts[2];

  verts[0] = lvert(x0, y0, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  verts[1] = lvert(x1, y1, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  ltex_bind(0, 0, 0);
  lvert_draw(verts, 2, R_LINES);
}

void lgfx_drawrect(float x, float y, float width, float height)
{
  lvert_t verts[4];

  verts[0] = lvert(x, y, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  verts[1] = lvert(x+width, y, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  verts[2] = lvert(x, y+height, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  verts[3] = lvert(x+width, y+height, 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  ltex_bind(0, 0, 0);
  lvert_draw(verts, 4, R_TRIANGLE_STRIP);
}

void lgfx_drawoval(float x, float y, float width, float height)
{
  lvert_t verts[OVALPOINTS];
  float centerx, centery;
  float inc, halfwidth, halfheight;
  int i;

  centerx = x + width * 0.5f;
  centery = y + height * 0.5f;
  inc = 6.28318530718f / OVALPOINTS;
  halfwidth = width * 0.5f;
  halfheight = height * 0.5f;
  for (i = 0; i < OVALPOINTS; ++i)
  {
    verts[i] = lvert(centerx + ((float)cos(i * inc) * halfwidth), centery + ((float)sin(i * inc) * halfheight), 0, 0, 0, -1, 0, 0, 1, 1, 1, 1);
  }
  ltex_bind(0, 0, 0);
  lvert_draw(verts, OVALPOINTS, R_TRIANGLE_FAN);
}

int lgfx_multitexture_supported() {
  return glActiveTexture != NULL && glClientActiveTexture != NULL;
}

int lgfx_mipmapping_supported() {
  return glGenerateMipmap != NULL;
}

/* texture */

ltex_t* ltex_alloc(int width, int height, int filter)
{
  GLuint gltex;
  GLint minfilter, magfilter;
  ltex_t* tex;

  /* get filter modes */
  if (filter == F_MIPMAP && !lgfx_mipmapping_supported()) filter = F_LINEAR;
  switch (filter)
  {
    case F_NONE:
      minfilter = GL_NEAREST;
      magfilter = GL_NEAREST;
      break;
    case F_LINEAR:
      minfilter = GL_LINEAR;
      magfilter = GL_LINEAR;
      break;
    case F_MIPMAP:
      minfilter = GL_LINEAR_MIPMAP_LINEAR;
      magfilter = GL_LINEAR;
      break;
  }

  /* generate opengl texture */
  glGenTextures(1, &gltex);
  glBindTexture(GL_TEXTURE_2D, gltex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
  glBindTexture(GL_TEXTURE_2D, 0);

  /* generate tex object */
  tex = (ltex_t*)malloc(sizeof(ltex_t));
  tex->glid = (int)gltex;
  tex->width = width;
  tex->height = height;
  tex->filter = filter;

  return tex;
}

void ltex_free(ltex_t* tex)
{
  if (tex && tex->glid != 0) glDeleteTextures(1, (const GLuint*)&tex->glid);
  free(tex);
}

void ltex_setpixels(const ltex_t* tex, const unsigned char* pixels)
{
  glBindTexture(GL_TEXTURE_2D, (GLuint)tex->glid);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  if (tex->filter == F_MIPMAP) glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ltex_getpixels(const ltex_t* tex, unsigned char* out_pixels)
{
  glBindTexture(GL_TEXTURE_2D, (GLuint)tex->glid);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out_pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ltex_draw(const ltex_t* tex, float x, float y)
{
  ltex_drawrot(tex, x, y, 0, 0, 0);
}

void ltex_drawrot(const ltex_t* tex, float x, float y, float angle, float pivotx, float pivoty)
{
  ltex_drawrotsized(tex, x, y, angle, pivotx, pivoty, (float)tex->width, (float)tex->height, 0, 0, 1, 1);
}

void ltex_drawrotsized(const ltex_t* tex, float x, float y, float angle, float pivotx, float pivoty, float width, float height, float u0, float v0, float u1, float v1)
{
  lvert_t verts[4];

  verts[0] = lvert(-pivotx, -pivoty, 0, 0, 0, -1, u0, v0, 1, 1, 1, 1);
  verts[1] = lvert(1-pivotx, -pivoty, 0, 0, 0, -1, u1, v0, 1, 1, 1, 1);
  verts[2] = lvert(-pivotx, 1-pivoty, 0, 0, 0, -1, u0, v1, 1, 1, 1, 1);
  verts[3] = lvert(1-pivotx, 1-pivoty, 0, 0, 0, -1, u1, v1, 1, 1, 1, 1);
  glPushMatrix();
  glTranslatef(x, y, 0);
  glRotatef(angle, 0, 0, -1);
  glScalef(width, height, 1);
  ltex_bind(tex, 0, 0);
  lvert_draw(verts, 4, R_TRIANGLE_STRIP);
  glPopMatrix();
}

void ltex_bind(const ltex_t* tex, const ltex_t* lightmap, int use_envlights)
{
  if (lgfx_multitexture_supported())
  {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, use_envlights ? GL_ADD : GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, lightmap ? lightmap->glid : 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex ? (GLuint)tex->glid : 0);
    glActiveTexture(GL_TEXTURE0);
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, tex ? (GLuint)tex->glid : 0);
  }
}

/* vertex */

static GLenum _lgfx_pickglrendermode(lrendermode_t mode)
{
  switch (mode)
  {
  case R_POINTS:
    return GL_POINTS;
  case R_LINES:
    return GL_LINES;
  case R_TRIANGLES:
    return GL_TRIANGLES;
  case R_TRIANGLE_STRIP:
    return GL_TRIANGLE_STRIP;
  case R_TRIANGLE_FAN:
    return GL_TRIANGLE_FAN;
  default:
    return GL_TRIANGLES;
  }
}

lvert_t lvert(float x, float y, float z, float nx, float ny, float nz, float u, float v, float r, float g, float b, float a)
{
  lvert_t vert;

  vert.pos[0] = x;
  vert.pos[1] = y;
  vert.pos[2] = z;
  vert.nor[0] = nx;
  vert.nor[1] = ny;
  vert.nor[2] = nz;
  vert.tex[0] = u;
  vert.tex[1] = v;
  vert.tex2[0] = u;
  vert.tex2[1] = v;
  vert.col[0] = r;
  vert.col[1] = g;
  vert.col[2] = b;
  vert.col[3] = a;
  return vert;
}

void lvert_draw(const lvert_t* vertices, unsigned int count, lrendermode_t mode)
{
  glVertexPointer(3, GL_FLOAT, sizeof(lvert_t), &vertices->pos[0]);
  glNormalPointer(GL_FLOAT, sizeof(lvert_t), &vertices->nor[0]);
  if (lgfx_multitexture_supported())
  {
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex[0]);
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex2[0]);
    glClientActiveTexture(GL_TEXTURE0);
  }
  else
  {
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex[0]);
  }
  glColorPointer(4, GL_FLOAT, sizeof(lvert_t), &vertices->col[0]);
  glDrawArrays(_lgfx_pickglrendermode(mode), 0, count);
}

void lvert_drawindexed(const lvert_t* vertices, const unsigned short* indices, unsigned int count, lrendermode_t mode)
{
  glVertexPointer(3, GL_FLOAT, sizeof(lvert_t), &vertices->pos[0]);
  glNormalPointer(GL_FLOAT, sizeof(lvert_t), &vertices->nor[0]);
  if (lgfx_multitexture_supported())
  {
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex2[0]);
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex[0]);
    glClientActiveTexture(GL_TEXTURE0);
  }
  else
  {
    glTexCoordPointer(2, GL_FLOAT, sizeof(lvert_t), &vertices->tex[0]);
  }
  glColorPointer(4, GL_FLOAT, sizeof(lvert_t), &vertices->col[0]);
  glDrawElements(_lgfx_pickglrendermode(mode), count, GL_UNSIGNED_SHORT, indices);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LITE_GFX_IMPLEMENTATION */
