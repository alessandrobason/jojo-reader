#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
TODO
 - draw shapes
 - draw text
 - audio
 - input
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#include <stdint.h>
#include <stdbool.h>

typedef unsigned char uchar;
typedef unsigned int uint;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct {
    float x, y;
} vec2;

typedef struct {
    float x, y, z;
} vec3;

typedef struct {
    float x, y, z, w;
} vec4;

typedef struct {
    float x, y, w, h;
} Rect;

typedef struct {
    float m11, m12;
    float m21, m22;
    float m31, m32;
} matrix;

typedef struct {
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;
} mat4;

typedef struct {
    uchar r, g, b, a;
} Colour;

typedef struct {
    uchar *data;
    int width, height;
} Image;

typedef struct {
    uint id;
} Texture;

typedef struct {
    Texture tex;
    Rect uv;
    Colour tint;
    vec2 pos;
    vec2 size;
    vec2 scale;
    float rotation;
    vec2 origin;
} Sprite;

typedef struct {
    Texture tex;
    Rect *uvs;
    uint frame_count;
    uint cur_frame;
    Colour tint;
    vec2 pos;
    vec2 size;
    vec2 scale;
    float rotation;
    vec2 origin;
} AnimSprite;

typedef struct {
    Texture tileset;
    int *tiles;
    uint width;
    uint height;
} Tilemap;

typedef struct {
    vec2 offset;
    vec2 target;
    float rotation;
    float zoom;
} Camera;

#ifdef __cplusplus
    #define CLITERAL(type) type
#else
    #define CLITERAL(type) (type)
#endif

#define colourWhite() CLITERAL(Colour){ 255, 255, 255, 255 }
#define colourBlack() CLITERAL(Colour){ 0,   0,   0,   255 }
#define colourRed()   CLITERAL(Colour){ 255, 0,   0,   255 }
#define colourGreen() CLITERAL(Colour){ 0,   255, 0,   255 }
#define colourBlue()  CLITERAL(Colour){ 0,   0,   255, 255 }

void initFramework(void);
void cleanupFramework(void);

double frameTime(void);
double timeSinceStart(void);

void setBackgroundColour(Colour colour);
void beginFrame(void);
void endFrame(void);
void beginCamera(Camera camera);
void endCamera(void);

Sprite loadSprite(const char *filename);
void freeSprite(Sprite sprite);

AnimSprite loadAnimSprite(const char *filename, uint hframes, uint vframes);
void freeAnimSprite(AnimSprite sprite);

Image loadImage(const char *filename);
Image loadImageFromMemory(const uchar *data, uint len);
void freeImage(Image image);

Texture loadTexture(const char *filename);
Texture loadTextureFromImage(Image image);
void freeTexture(Texture texture);

void drawTexture(Texture texture, vec2 position);
void drawTexturePro(Texture texture, Colour tint, Rect uv, vec2 pos, vec2 size, vec2 scale, float rotation, vec2 origin);
void drawTextureMat(Texture texture, Colour tint, Rect uv, vec2 size, matrix mat);
void drawSprite(Sprite sprite);
void drawAnimSprite(AnimSprite sprite);
void drawTilemap(Tilemap tilemap, Colour tint, vec2 offset);

#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

#ifndef MATH_NO_MACROS
#define vec2Zero() (CLITERAL(vec2){ 0, 0 })
#define vec2One()  (CLITERAL(vec2){ 1, 1 })
#define vec3Zero() (CLITERAL(vec3){ 0, 0, 0 })
#define vec3One()  (CLITERAL(vec3){ 1, 1, 1 })
#define vec4Zero() (CLITERAL(vec4){ 0, 0, 0, 0 })
#define vec4One()  (CLITERAL(vec4){ 1, 1, 1, 1 })

#define matIdentity()             (CLITERAL(matrix){ 1, 0, 0, 1, 0, 0 })
#define matFromPosition(position) (CLITERAL(matrix){ 1, 0, 0, 1, position.x, position.y })
#define matFromPositionV(x, y)    (CLITERAL(matrix){ 1, 0, 0, 1, x, y })
#define matFromScale(scale)       (CLITERAL(matrix){ scale.x, 0, 0, scale.y, 0, 0 })
#define matFromScaleV(sx, sy)     (CLITERAL(matrix){ sx, 0, 0, sy, 0, 0 })
#define matFromRotation(rotation) (CLITERAL(matrix){ cosf(rotation), sinf(rotation), -sinf(rotation), cosf(rotation), 0, 0 })
#else
inline vec2 vec2Zero(void) { return CLITERAL(vec2){0, 0}; }
inline vec2 vec2One(void)  { return CLITERAL(vec2){1, 1}; }
inline vec3 vec3Zero(void) { return CLITERAL(vec3){0, 0, 0}; }
inline vec3 vec3One(void)  { return CLITERAL(vec3){1, 1, 1}; }
inline vec4 vec4Zero(void) { return CLITERAL(vec4){0, 0, 0, 0}; }
inline vec4 vec4One(void)  { return CLITERAL(vec4){1, 1, 1, 1}; }

matrix matIdentity(void);
matrix matFromPosition(vec2 position);
matrix matFromScale(vec2 scale);
matrix matFromRotation(float rotation);
#endif
matrix matFromRotationOrigin(float angle, vec2 origin);
matrix matFromTransform(vec2 pos, vec2 scale, float rot, vec2 origin);
matrix matSum(matrix a, matrix b);
matrix matSub(matrix a, matrix b);
matrix matMul(matrix a, matrix b);
bool matAreEqual(matrix a, matrix b, float epsilon);

vec2 posFromMat(matrix mat);
vec2 scaleFromMat(matrix mat);
bool matHasRotation(matrix mat);
float rotFromMat(matrix mat);

mat4 mat4FromMatrix(matrix m);
mat4 mat4Ortho(float left, float right, float top, float bottom, float znear, float zfar);
mat4 mat4Mul(mat4 a, mat4 b);
mat4 mat4Transpose(mat4 m);

Rect rectMove(Rect rect, vec2 amount);
Rect rectExpand(Rect rect, vec2 amount);
Rect rectScale(Rect rect, vec2 scale);

#ifdef __cplusplus
} // extern "C"
#endif