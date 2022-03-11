#ifndef TOPHAT_H
#define TOPHAT_H

#include <stdbool.h>
#include <miniaudio.h>
#include <stb_truetype.h>
#include <umka_api.h>
#include <GL/gl.h>

#define MAX_SOUNDS 512
#define MAX_IMAGES 2048
#define MAX_FONTS 128

typedef float fu;
typedef unsigned short uu;
typedef short iu;

#define SWAP_VARS(v1, v2, type) { type tmp = v1; v1 = v2; v2 = tmp; }
#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef union {
	struct {fu w, h;};
	struct {fu x, y;};
} th_vf2;

typedef struct {
	fu x, y, w, h;
} th_rect;

typedef union {
	struct {th_vf2 tl, tr, br, bl;};
	th_vf2 v[4];
} th_quad;

typedef struct {
	th_vf2 pos;
	th_vf2 scale;
	th_vf2 origin;
	fu rot;
} th_transform;

typedef struct {
	int playing, looping, seek_to;
	fu volume;
	ma_decoder decoder;
} th_sound;

typedef struct {
	th_vf2 dm;
	uu channels;
	uint32_t *data;
	unsigned int gltexture;
	uu filter;
	th_rect crop;
	char flipv;
	char fliph;
} th_image;

typedef struct {
	int x;
	int y;
	int *v;
	int w;
	int h;
	int vc;
} th_poly;

typedef struct {
	th_rect rect;
	uint64_t img;
	th_transform t;
	uint32_t color;
} th_ent;

typedef struct {
	uu index;
	th_vf2 pos;
} th_coll;

typedef struct {
	uint32_t *dots;
	uu w, h;
	fu rect_size;
	uint32_t color;
} th_lightmask;

typedef struct {
	th_vf2 pos;
	uu radius;
	uint32_t tint;
} th_spotlight;

typedef struct {
	uint64_t start_time;
	int seed;
} _th_particle;

typedef struct {
	th_vf2 pos;
	th_vf2 dm;
	th_vf2 gravity;
	bool repeat;
	bool active;

	th_vf2 angle;

	uu lifetime;
	fu lifetime_randomness;

	fu velocity;
	fu velocity_randomness;

	fu size;
	fu size_randomness;
	fu max_size;

	fu rotation;
	fu max_rotation;
	fu rotation_randomness;

	UmkaDynArray(uint32_t) colors;

	UmkaDynArray(_th_particle) particles;
} th_particles;

typedef struct {
	th_vf2 pos;
	fu l;
	fu r;
} th_ray;

typedef struct {
	uint64_t i;
	th_vf2 cs;
	th_vf2 dm;
} th_atlas;

typedef struct {
	th_atlas a;
	th_vf2 pos;
	uu w;
	UmkaDynArray(uu) cells;
	UmkaDynArray(bool) collmask;
	fu scale;
} th_tmap;

typedef struct {
	stbtt_fontinfo *info;
	unsigned char *buf;
} th_font;

// struct holding all tophat's global variables.
typedef struct {
	char respath[1024];
	fu scaling;
	void *umka;

	uu pressed[255];
	uu just_pressed[255];
	th_vf2 mouse;

	th_sound *sounds;
	uu sound_count;

	th_font *fonts;
	uu font_count;

	th_image *images;
	uu image_count;
} th_global;

// atlas
th_vf2 th_atlas_nth_coords(th_atlas *a, uu n);
th_rect th_atlas_get_cell(th_atlas *a, th_vf2 cell);

// audio
void th_audio_init();
void th_audio_deinit();
th_sound *th_sound_load(char *path);

// entity
th_quad th_ent_transform(th_ent *e);
void th_ent_draw(th_ent *o, th_rect *camera);
void th_ent_getcoll(th_ent *e, th_ent **scene, uu count, uu *collC,
	uu maxColls, th_coll *colls);

// image
th_image *th_load_image(char *path);
void th_free_image(th_image *img);
void th_image_from_data(th_image *img, uint32_t *data, th_vf2 dm);
void th_image_set_filter(th_image *img, int filter);
unsigned int th_gen_texture(uint32_t *data, th_vf2 dm, unsigned filter);
void th_blit_tex(th_image *img, th_transform t, uint32_t color);
void th_image_init();
void th_image_deinit();

// light
void th_lightmask_clear(th_lightmask *d);
void th_lightmask_draw(th_lightmask *d, th_rect *cam);
void th_spotlight_stamp(th_spotlight *l, th_lightmask *d);

// misc
float th_get_scaling(int w, int h, int camw, int camh);
void th_error(char *text, ...);
void th_rotate_point(th_vf2 *p, th_vf2 o, fu rot);
void th_vector_normalize(float *x, float *y);

// particles
void th_particles_draw(th_particles *p, th_rect cam, int t);

// raycast
int th_ray_getcoll(th_ray *ra, th_ent **scene, int count, th_vf2 *ic);

// tilemap
void th_tmap_draw(th_tmap *t, th_rect *cam);
void th_tmap_autotile(uu *tgt, uu *src, uu w, uu h, uu *tiles, uu limiter);

// tophat
th_image *th_get_image(uu index);
th_font *th_get_font(uu index);
th_sound *th_get_sound(uu index);

th_image *th_get_image_err(uu index);
th_font *th_get_font_err(uu index);
th_sound *th_get_sound_err(uu index);

th_image *th_alloc_image();
th_font *th_alloc_font();
th_sound *th_alloc_sound();

// font
th_font *th_font_load(char *path);
void th_str_to_img(
	th_image *out, th_font *font,
	uint32_t *runes, uu runec,
	fu scale, uint32_t color,
	th_vf2 spacing);
void th_font_deinit();

th_vf2 th_quad_min(th_quad q);

//// "unexported" functions

// audio
void _th_audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

// collisions
int th_line_to_line(th_vf2 b1, th_vf2 e1, th_vf2 b2, th_vf2 e2, th_vf2 *ic);
uu th_point_to_quad(th_vf2 p, th_quad *q, th_vf2 *ic);
uu th_ent_to_ent(th_ent *e1, th_ent *e2, th_vf2 *ic);
uu th_line_to_quad(th_vf2 b, th_vf2 e, th_quad *q, th_vf2 *ic);
uu _th_coll_on_tilemap(th_ent *e, th_tmap *t, uu *vert, th_vf2 *tc);
bool th_ray_to_tilemap(th_ray *ra, th_tmap *t, th_vf2 *ic);

// image
void _th_rdimg(th_image *img, unsigned char *data);

// light
void _th_lightmask_stamp_point(th_lightmask *d, int x, int y, uint32_t color);

void th_gl_init();
GLuint th_gl_compile_shader(const char **src, GLenum type);
GLuint th_gl_create_prog(const char *vert_src, const char *frag_src, const char **attribs, int nattribs);
void th_gl_free_prog(GLuint prog);

void th_input_key(int keycode, int bDown);
void th_input_cycle();

void th_window_setup(char *name, int w, int h);
void th_window_get_dimensions(int *w, int *h);
int th_window_handle();
void th_window_swap_buffers();
void th_window_clear_frame();

void _th_umka_bind(void *umka);

void th_canvas_rect(uint32_t color, th_rect r);
void th_canvas_init();
void th_canvas_line(uint32_t color, th_vf2 f, th_vf2 t, fu thickness);
void th_canvas_text(char *text, uint32_t color, th_vf2 p, fu size);
void th_canvas_triangle(uint32_t color, th_vf2 a, th_vf2 b, th_vf2 c);
void th_canvas_flush();

#endif
