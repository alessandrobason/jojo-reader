#include "framework.h"

// stdlib
#include <math.h>
#include <assert.h>

// sokol
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>

// stb
#include <stb_image.h>

#include "../tracelog.h"
#include "base.glsl.h"

#define COLOUR_TO_NORM(c) { (c).r/255.f, (c).g/255.f, (c).b/255.f, (c).a/255.f }

#define REPEAT_N(n, fn) do { static int __repeat_##n = n; if(__repeat_##n > 0) { __repeat_##n--; fn; } } while(0)

enum {
    BATCH_SIZE = 512,
    // for most of our use (quads) we'll need 4 vertices and 6 indices
    MAX_VERTICES = BATCH_SIZE * 4,
    MAX_INDICES = BATCH_SIZE * 6
};

typedef struct {
    vec2 pos;
    vec2 tex;
    vec4 col;
} Vertex;

struct {
    float res_w;
    float res_h;
} options = {0};

static struct {
    Vertex vertices[MAX_VERTICES];
    u16 indices[MAX_INDICES];
    u16 vcount;
    u16 icount;
    Texture tex;
    Texture def_tex;
    matrix mat;
} batch = {0};

static struct {
    sg_shader shd;
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint64_t last_time;
    uint64_t dt;
} state = {0};

// checks if the batch needs to redraw
static bool batchCheckTexture(Texture tex);
static bool batchCheckMatrix(matrix mat);
static bool batchCheckTexAndMat(Texture tex, matrix mat);
// renders what is in the batch
static void batchDraw(void);
static void batchSetTexture(Texture tex);
static void batchSetMatrix(matrix mat);
static void batchSetTexAndMat(Texture tex, matrix mat);
static void batchPushTriangle(vec2 a, vec2 b, vec2 c, Colour tint);
static void batchPushTriangleTex(vec2 a, vec2 uva, vec2 b, vec2 uvb, vec2 c, vec2 uvc, Colour tint);
static void batchPushQuad(Rect rect, Rect uv, Colour tint);

void initFramework(void) {
    sg_setup(&(sg_desc){ .context = sapp_sgcontext()});
    stm_setup();
    //stbi_set_flip_vertically_on_load(true);

    options.res_w = 200;
    options.res_h = 200;

    batch.mat = matIdentity();

    state.last_time = stm_now();

    state.pass_action.colors[0] = (sg_color_attachment_action) {
        .action = SG_ACTION_CLEAR,
        .value = { 0.12f, 0.12f, 0.12f, 1.f }
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(batch.vertices),
        .usage = SG_USAGE_STREAM
    });

    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(batch.indices),
        .usage = SG_USAGE_STREAM
    });

    uint32_t pixels[2*2] = {
        0xFF000000, 0xFF000000,
        0xFF000000, 0xFF000000,
    };
    sg_image def = sg_make_image(&(sg_image_desc){
        .width = 2,
        .height = 2,
        .data.subimage[0][0] = SG_RANGE(pixels),
    });
    batch.def_tex = (Texture){ .id = def.id };

    state.shd = sg_make_shader(base_shader_desc(sg_query_backend()));
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = state.shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .cull_mode = SG_CULLMODE_BACK,
        .layout = {
            .attrs = {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_texc].format     = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_colour].format   = SG_VERTEXFORMAT_FLOAT4
            }
        }
    });
}

void cleanupFramework(void) {
    sg_shutdown();
}

double frameTime(void) {
    return stm_sec(stm_round_to_common_refresh_rate(state.dt));
}

double timeSinceStart(void) {
    return stm_sec(stm_round_to_common_refresh_rate(state.last_time));
}

void setBackgroundColour(Colour colour) {
    state.pass_action.colors[0].value = (sg_color)COLOUR_TO_NORM(colour);
}

void beginFrame(void) {
    state.dt = stm_laptime(&state.last_time);
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
}

void endFrame(void) {
    batchDraw();
    sg_end_pass();
    sg_commit();
}

void beginCamera(Camera camera) {
    batchDraw();
    batch.mat = matFromTransform(
        camera.target, 
        (vec2){ camera.zoom, camera.zoom }, 
        camera.rotation, 
        camera.offset
    );
}

void endCamera(void) {
    batchDraw();
}

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            LOADING FUNCTIONS            
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

Sprite loadSprite(const char *filename) {
    Image img = loadImage(filename);
    Sprite out = (Sprite) {
        .tex = loadTextureFromImage(img),
        .scale = vec2One(),
        .size = { (float)img.width, (float)img.height },
        .uv = { 0, 0, 1, 1 },
        .tint = colourWhite(),
        // TODO maybe add option for centered?
        // .origin = { (float)img.width / 2.f, (float)img.height / 2.f },
    };
    freeImage(img);
    return out;
}

void freeSprite(Sprite sprite) {
    freeTexture(sprite.tex);
}

AnimSprite loadAnimSprite(const char *filename, uint hframes, uint vframes) {
    Image img = loadImage(filename);

    uint sprite_w = (uint)img.width / hframes;
    uint sprite_h = (uint)img.height / vframes;

    AnimSprite out = (AnimSprite){
        .tex = loadTextureFromImage(img),
        .uvs = malloc(sizeof(Rect) * hframes * vframes),
        .frame_count = hframes * vframes,
        .scale = vec2One(),
        .tint = colourWhite(),
        .size = { (float)sprite_w, (float)sprite_h },
        // TODO maybe add option for centered?
        // .origin = { (float)img.width / 2.f, (float)img.height / 2.f },
    };
    freeImage(img);

    float tile_w = 1.f / (float)hframes;
    float tile_h = 1.f / (float)vframes;

    for(uint y = 0; y < vframes; ++y) {
        for(uint x = 0; x < hframes; ++x) {
            out.uvs[x + y * vframes] = (Rect){
                .x = (float)(tile_w * x),
                .y = (float)(tile_h * y),
                .w = (float)tile_w,
                .h = (float)tile_h
            };
        }
    }

    return out;
}

void freeAnimSprite(AnimSprite sprite) {
    freeTexture(sprite.tex);
    free(sprite.uvs);
}

Image loadImage(const char *filename) {
    Image out;
    int channels;
    out.data = stbi_load(filename, &out.width, &out.height, &channels, 4);
    assert(out.data != NULL);
    return out;
}

Image loadImageFromMemory(const uchar *data, uint len) {
    Image out;
    int channels;
    out.data = stbi_load_from_memory(data, len, &out.width, &out.height, &channels, 4);
    if (out.data == NULL) {
        err("stbi error: %s", stbi_failure_reason());
    }
    assert(out.data != NULL);
    return out;
}

void freeImage(Image image) {
    stbi_image_free(image.data);
}

Texture loadTexture(const char *filename) {
    Image img = loadImage(filename);
    Texture tex = loadTextureFromImage(img);
    freeImage(img);
    return tex;
}

Texture loadTextureFromImage(Image image) {
    sg_image tex = sg_make_image(&(sg_image_desc){
        .width = image.width,
        .height = image.height,
        .data.subimage[0][0] = { image.data, image.width * image.height * 4 }
    });
    assert(tex.id != 0);
    return (Texture){ tex.id };
}

void freeTexture(Texture texture) {
    sg_destroy_image((sg_image){texture.id});
}

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
            BATCH FUNCTIONS            
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

static bool batchCheckTexture(Texture tex) {
    return batch.tex.id != tex.id;
}

static bool batchCheckMatrix(matrix mat) {
    return matAreEqual(batch.mat, mat, 0.000001f);
}

static bool batchCheckTexAndMat(Texture tex, matrix mat) {
    return batchCheckTexture(tex) || batchCheckMatrix(mat);
}

static void batchDraw(void) {
    // early return in case theres nothing to draw
    if (batch.vcount == 0) return;

    const float multiplier = 5.f;
    const float tile_sz = 16.f;
    const float ratio = multiplier / tile_sz;
    options.res_w = sapp_widthf() * ratio;
    options.res_h = sapp_heightf() * ratio;

    vs_params_t params = {
        .transform = mat4Mul(
            mat4FromMatrix(batch.mat), 
            mat4Ortho(0, options.res_w, 0, options.res_h, 0, 1)
        )
    };

    sg_update_buffer(state.bind.vertex_buffers[0], &(sg_range){ batch.vertices, sizeof(Vertex) * batch.vcount });
    sg_update_buffer(state.bind.index_buffer, &(sg_range){ batch.indices, sizeof(u16) * batch.icount });

    if (batch.tex.id == SG_INVALID_ID)
        state.bind.fs_images[SLOT_tex] = (sg_image){ batch.def_tex.id };
    else
        state.bind.fs_images[SLOT_tex] = (sg_image){ batch.tex.id };

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(params));

    sg_draw(0, batch.icount, 1);

    batch.vcount = 0;
    batch.icount = 0;
    batch.tex.id = SG_INVALID_ID;
    batch.mat = matIdentity();
}

static void batchSetTexture(Texture tex) {
    if (batchCheckTexture(tex)) {
        batchDraw();
        batch.tex = tex;
    }
}

static void batchSetMatrix(matrix mat) {
    if (batchCheckMatrix(mat)) {
        batchDraw();
        batch.mat = mat;
    }
}

static void batchSetTexAndMat(Texture tex, matrix mat) {
    if (batchCheckTexAndMat(tex, mat)) {
        batchDraw();
        batch.tex = tex;
        batch.mat = mat;
    }
}

static void batchPushTriangle(vec2 a, vec2 b, vec2 c, Colour tint) {
    Vertex *vbuf = &batch.vertices[batch.vcount];
    u16 *ibuf = &batch.indices[batch.icount];

    vec4 col = COLOUR_TO_NORM(tint);
    vbuf[0] = (Vertex){ a, {0}, col };
    vbuf[1] = (Vertex){ b, {0}, col };
    vbuf[2] = (Vertex){ c, {0}, col };

    ibuf[0] = batch.vcount;
    ibuf[1] = batch.vcount + 1;
    ibuf[2] = batch.vcount + 2;

    batch.vcount += 3;
    batch.icount += 3;
}

static void batchPushTriangleTex(vec2 a, vec2 uva, vec2 b, vec2 uvb, vec2 c, vec2 uvc, Colour tint) {
    Vertex *vbuf = &batch.vertices[batch.vcount];
    u16 *ibuf = &batch.indices[batch.icount];

    vec4 col = COLOUR_TO_NORM(tint);
    vbuf[0] = (Vertex){ a, uva, col };
    vbuf[1] = (Vertex){ b, uvb, col };
    vbuf[2] = (Vertex){ c, uvc, col };

    ibuf[0] = batch.vcount;
    ibuf[1] = batch.vcount + 1;
    ibuf[2] = batch.vcount + 2;

    batch.vcount += 3;
    batch.icount += 3;
}

static void batchPushQuad(Rect rect, Rect uv, Colour tint) {
    Vertex *vbuf = &batch.vertices[batch.vcount];
    u16 *ibuf = &batch.indices[batch.icount];

    vec4 col = COLOUR_TO_NORM(tint);
    vbuf[0] = (Vertex){ { rect.x,          rect.y },          { uv.x,        uv.y + uv.h }, col };
    vbuf[1] = (Vertex){ { rect.x + rect.w, rect.y },          { uv.x + uv.w, uv.y + uv.h }, col };
    vbuf[2] = (Vertex){ { rect.x + rect.w, rect.y + rect.h }, { uv.x + uv.w, uv.y },        col };
    vbuf[3] = (Vertex){ { rect.x,          rect.y + rect.h }, { uv.x,        uv.y },        col };

    ibuf[0] = batch.vcount;
    ibuf[1] = batch.vcount + 1;
    ibuf[2] = batch.vcount + 2;
    ibuf[3] = batch.vcount + 2;
    ibuf[4] = batch.vcount + 3;
    ibuf[5] = batch.vcount;

    batch.vcount += 4;
    batch.icount += 6;
}

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
            RENDERING FUNCTIONS            
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

static void __drawTextureNoRot(Texture texture, Colour tint, Rect uv, vec2 pos, vec2 size, vec2 scale, vec2 origin) {
    size.x *= scale.x;
    size.y *= scale.y;

    pos.x -= (origin.x * scale.x);
    pos.y -= (origin.y * scale.y);

    batchSetTexture(texture);
    batchPushQuad((Rect){pos.x, pos.y, size.x, size.y}, uv, tint);
}

void drawTexture(Texture texture, vec2 position) {
    sg_image_info info = sg_query_image_info((sg_image){texture.id});
    vec2 size = { (float)info.width, (float)info.height };
    __drawTextureNoRot(texture, colourWhite(), (Rect){0, 0, 1, 1}, position, size, vec2One(), vec2Zero());
}

void drawTexturePro(Texture texture, Colour tint, Rect uv, vec2 pos, vec2 size, vec2 scale, float rotation, vec2 origin) {
    if(rotation != 0.f) {
        matrix mat = matFromTransform(pos, scale, rotation, origin);
        drawTextureMat(texture, tint, uv, size, mat);
    }
    else {
        __drawTextureNoRot(texture, tint, uv, pos, size, scale, origin);
    }
}

void drawTextureMat(Texture texture, Colour tint, Rect uv, vec2 size, matrix mat) {
    batchSetTexAndMat(texture, mat);
    batchPushQuad((Rect){0, 0, size.x, size.y}, uv, tint);
}

void drawSprite(Sprite sprite) {
    if(sprite.rotation != 0.f)
        drawTexturePro(sprite.tex, sprite.tint, sprite.uv, sprite.pos, sprite.size, sprite.scale, sprite.rotation, sprite.origin);
    else
        __drawTextureNoRot(sprite.tex, sprite.tint, sprite.uv, sprite.pos, sprite.size, sprite.scale, sprite.origin);
}

void drawAnimSprite(AnimSprite sprite) {
    if(sprite.rotation != 0.f)
        drawTexturePro(sprite.tex, sprite.tint, sprite.uvs[sprite.cur_frame], sprite.pos, sprite.size, sprite.scale, sprite.rotation, sprite.origin);
    else
        __drawTextureNoRot(sprite.tex, sprite.tint, sprite.uvs[sprite.cur_frame], sprite.pos, sprite.size, sprite.scale, sprite.origin);
}

void drawTilemap(Tilemap tilemap, Colour tint, vec2 offset) {
    sg_image_info info = sg_query_image_info((sg_image){tilemap.tileset.id});
    float norm_w = 1.f / (float)tilemap.width;
    float norm_h = 1.f / (float)tilemap.height;
    Rect uv = { 
        .w = norm_w, 
        .h = norm_h 
    };
    vec2 tile_size = (vec2){
        .x = (float)((uint)info.width / tilemap.width),
        .y = (float)((uint)info.height / tilemap.height)
    };
    
    for(uint y = 0; y < tilemap.height; ++y) {
        for(uint x = 0; x < tilemap.width; ++x) {
            int id = tilemap.tiles[x + y * tilemap.width];
            if(id < 0) continue;
            uv.x = (float)((id % tilemap.width) * norm_w);
            uv.y = (float)((id / tilemap.width) * norm_h);

            vec2 pos = offset;
            pos.x += (float)tilemap.width;
            pos.y += (float)tilemap.height;

            __drawTextureNoRot(tilemap.tileset, tint, uv, pos, tile_size, vec2One(), vec2Zero());
        }
    }
}

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
-- for future reference 'cause i'm bad at math
  3x2 matrix (3 rows, 2 columns)
  for 2D rendering it's all we need
  
  # identity
  [ 1, 0 ]
  [ 0, 1 ]
  [ 0, 0 ]
  
  # translation
  [ 1, 0 ]
  [ 0, 1 ]
  [ x, y ]
  
  # scale
  [ x, 0 ]
  [ 0, y ]
  [ 0, 0 ]
  
  # rotation
  c, s -> cosine, sine of rotation (in radians)
  [  c, s  ]
  [ -s, c  ]
  [  0, 0  ]
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifndef MATH_NO_MACROS
matrix matFromRotationOrigin(float angle, vec2 origin) {
    if (origin.x != 0 || origin.y != 0) {
        return matMul(matFromPositionV(-origin.x, -origin.y), matFromRotation(angle));
    }
    else {
        return matFromRotation(angle);
    }
}

matrix matFromTransform(vec2 pos, vec2 scale, float rot, vec2 origin) {
    matrix res = matIdentity();
    if (origin.x != 0 || origin.y != 0)
        res = matMul(res, matFromPositionV(-origin.x, -origin.y));
    if(scale.x != 1 || scale.y != 1)
        res = matMul(res, matFromScale(scale));
    if(rot != 0)
        res = matMul(res, matFromRotation(rot));
    if(pos.x != 0 || pos.y != 0)
        res = matMul(res, matFromPosition(pos));
    return res;
}
#else
matrix matIdentity(void) {
    return (matrix){ 1, 0, 0, 1, 0, 0 };
}

matrix matFromPosition(vec2 position) {
    return (matrix){ 1, 0, 0, 1, position.x, position.y };
}

matrix matFromScale(vec2 scale) {
    return (matrix){ scale.x, 0, 0, scale.y, 0, 0 };
}

matrix matFromRotation(float rotation) {
    float c = cosf(rotation);
    float s = sinf(rotation);
    return (matrix){ c, s, -s, c, 0, 0 };
}
#endif

matrix matSum(matrix a, matrix b) {
    return (matrix) {
        a.m11 + b.m11, a.m12 + b.m12,
        a.m21 + b.m21, a.m22 + b.m22,
        a.m31 + b.m31, a.m32 + b.m32
    };
}

matrix matSub(matrix a, matrix b) {
    return (matrix) {
        a.m11 - b.m11, a.m12 - b.m12,
        a.m21 - b.m21, a.m22 - b.m22,
        a.m31 - b.m31, a.m32 - b.m32
    };
}

matrix matMul(matrix a, matrix b) {
    // cross product
    return (matrix) {
        a.m11 * b.m11 + a.m12 * b.m21,
        a.m11 * b.m12 + a.m12 * b.m22,
        a.m21 * b.m11 + a.m22 * b.m21,
        a.m21 * b.m12 + a.m22 * b.m22,
        a.m31 * b.m11 + a.m32 * b.m21 + b.m31,
        a.m31 * b.m12 + a.m32 * b.m22 + b.m32
    };
}

bool matAreEqual(matrix a, matrix b, float epsilon) {
    return fabsf(a.m11 - b.m11) < epsilon &&
           fabsf(a.m12 - b.m12) < epsilon &&
           fabsf(a.m21 - b.m21) < epsilon &&
           fabsf(a.m22 - b.m22) < epsilon &&
           fabsf(a.m31 - b.m31) < epsilon &&
           fabsf(a.m32 - b.m32) < epsilon;
}

vec2 posFromMat(matrix mat) {
    return (vec2){ mat.m31, mat.m32 };
}

vec2 scaleFromMat(matrix mat) {
    if (matHasRotation(mat)) {
        assert(false && "can't get scale from matrix if it has rotation");
    }
    return (vec2){ mat.m11, mat.m22 };
}

bool matHasRotation(matrix mat) {
    // when sin(angle) = 0 -> angle = 0
    return mat.m21 != 0;
}

float rotFromMat(matrix mat) {
    if (matHasRotation(mat))
        return asinf(-mat.m21);
    return 0.f;
}

mat4 mat4FromMatrix(matrix m) {
    return (mat4) {
        m.m11, m.m12, 0, 0,
        m.m21, m.m22, 0, 0,
        0,     0,     1, 0,
        m.m31, m.m32, 0, 1,
    };
}

mat4 mat4Ortho(float left, float right, float top, float bottom, float znear, float zfar) {
    mat4 out = {0};
    out.m11 = 2 / (right - left);
    out.m22 = 2 / (top - bottom);
    out.m33 = 1 / (znear - zfar);
    out.m41 = (left + right) / (left - right);
    out.m42 = (top + bottom) / (bottom - top);
    out.m43 = znear / (znear - zfar);
    out.m44 = 1;
    return out;
}

mat4 mat4Mul(mat4 a, mat4 b) {
    return (mat4){
        .m11 = a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31 + a.m14 * b.m41,
        .m12 = a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32 + a.m14 * b.m42,
        .m13 = a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33 + a.m14 * b.m43,
        .m14 = a.m11 * b.m14 + a.m12 * b.m24 + a.m13 * b.m34 + a.m14 * b.m44,
        
        .m21 = a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31 + a.m24 * b.m41,
        .m22 = a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32 + a.m24 * b.m42,
        .m23 = a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33 + a.m24 * b.m43,
        .m24 = a.m21 * b.m14 + a.m22 * b.m24 + a.m23 * b.m34 + a.m24 * b.m44,
        
        .m31 = a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31 + a.m34 * b.m41,
        .m32 = a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32 + a.m34 * b.m42,
        .m33 = a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33 + a.m34 * b.m43,
        .m34 = a.m31 * b.m14 + a.m32 * b.m24 + a.m33 * b.m34 + a.m34 * b.m44,
        
        .m41 = a.m41 * b.m11 + a.m42 * b.m21 + a.m43 * b.m31 + a.m44 * b.m41,
        .m42 = a.m41 * b.m12 + a.m42 * b.m22 + a.m43 * b.m32 + a.m44 * b.m42,
        .m43 = a.m41 * b.m13 + a.m42 * b.m23 + a.m43 * b.m33 + a.m44 * b.m43,
        .m44 = a.m41 * b.m14 + a.m42 * b.m24 + a.m43 * b.m34 + a.m44 * b.m44
    };
}

mat4 mat4Transpose(mat4 m) {
    return (mat4){
        .m11 = m.m11, .m12 = m.m21, .m13 = m.m31, .m14 = m.m41,
        .m21 = m.m12, .m22 = m.m22, .m23 = m.m32, .m24 = m.m42,
        .m31 = m.m13, .m32 = m.m23, .m33 = m.m33, .m34 = m.m43,
        .m41 = m.m14, .m42 = m.m24, .m43 = m.m34, .m44 = m.m44
    };
}

Rect rectMove(Rect rect, vec2 amount) {
    return (Rect) { 
        .x = rect.x + amount.x,
        .y = rect.y + amount.y,
        .w = rect.w,
        .h = rect.h
    };
}

Rect rectExpand(Rect rect, vec2 amount) {
    return (Rect) { 
        .x = rect.x,
        .y = rect.y,
        .w = rect.w + amount.x,
        .h = rect.h + amount.y
    };
}

Rect rectScale(Rect rect, vec2 scale) {
    return (Rect) {
        .x = rect.x,
        .y = rect.y,
        .w = rect.w * scale.x,
        .h = rect.h * scale.y
    };
}