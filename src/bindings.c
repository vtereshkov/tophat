#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <GL/gl.h>

#include <chew.h>
#include <umka_api.h>
#include <stb_image.h>

#include "tophat.h"

extern th_global thg;

extern char *th_em_modulesrc[];

static
char *conv_path(char *out, char *path) {
	if (strstr(path, "raw://") == path) {
		strcpy(out, path + strlen("raw://"));
	} else {
		strcpy(out, thg.respath);
		strcat(out, path);
	}
	return out;
}

void umfopen(UmkaStackSlot *p, UmkaStackSlot *r) {
	char *name = (char *)p[1].ptrVal;
	const char *mode = (const char *)p[0].ptrVal;

	char path[512];
	FILE *f = fopen(conv_path(path, name), mode);
	r->ptrVal = f;
}

void umfonttexttoimg(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_font *f = th_get_font_err(p[5].uintVal);
	if (!f)
		return;

	uint32_t *runes = (uint32_t *)p[4].ptrVal;
	uu runec = p[3].intVal;
	fu scale = p[2].real32Val;
	uint32_t color = p[1].uintVal;
	th_vf2 spacing = *(th_vf2 *)&p[0];

	if (thg.image_count >= MAX_IMAGES - 1) {
		th_error("Too many images. Create an issue.");
		return;
	}
	th_image *img = th_alloc_image();
	if (!img)
		return;
	th_str_to_img(img, f, runes, runec, scale, color, spacing);
	r->intVal = thg.image_count;
}

void umfontload(UmkaStackSlot *p, UmkaStackSlot *r) {
	char buf[1024];
	th_font *f = th_font_load(conv_path(buf, p[0].ptrVal));
	if (!f)
		r->intVal = 0;
	r->intVal = thg.font_count;
}

void umfontgetyoff(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_font *f = th_get_font_err(p[1].uintVal);
	if (!f) return;
	uint32_t rune = p[0].uintVal;

	int out, t;
	stbtt_GetCodepointBitmapBox(f->info, rune, 1, 1, &t, &out, &t, &t);
	r->intVal = out;
}

// sets values of all dots to lightmask's color
void umlightmaskclear(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_lightmask *l = (th_lightmask *)p[0].ptrVal;

	th_lightmask_clear(l);
}

// draws the lightmask
void umlightmaskdraw(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_lightmask *l = (th_lightmask *)p[1].ptrVal;
	th_rect *cam = (th_rect *)p[0].ptrVal;

	th_lightmask_draw(l, cam);
}

// "stamps" the spotlight on the mask
void umspotlightstamp(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_rect *cam = (th_rect *)p[0].ptrVal;
	th_lightmask *l = (th_lightmask *)p[1].ptrVal;
	th_spotlight *s = (th_spotlight *)p[2].ptrVal;

	int x = s->pos.x, y = s->pos.x;
	s->pos.x -= (cam->x - cam->w/2);
	s->pos.y -= (cam->y - cam->h/2);

	th_spotlight_stamp(s, l);

	s->pos.x = x;
	s->pos.y = y;
}

///////////////////////
// particles
void umparticlesdraw(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_particles *emitter = (th_particles *)p[2].ptrVal;
	th_rect *cam = (th_rect *)p[1].ptrVal;
	int t = p[0].intVal;

	th_particles_draw(emitter, *cam, t);
}

///////////////////////
// tilemaps
// draws a tilemap takes a rectangle as a camera and the tilemap itself
void umdrawtmap(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_rect *cam = (th_rect *)p[0].ptrVal;
	th_tmap *t = (th_tmap *)p[1].ptrVal;
	th_tmap_draw(t, cam);
}

// checks, if tilemap collides with entity.
// ent - entity to collide with, t - tilemap, x and y - pointers to ints used to return, where the collision occured
void umtmapgetcoll(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_ent *ent = (th_ent *)p[0].ptrVal;
	th_tmap *t = (th_tmap *)p[1].ptrVal;
	uu *vert = (uu *)p[2].ptrVal;
	th_vf2 *tc = (th_vf2 *)p[3].ptrVal;

	r->intVal = _th_coll_on_tilemap(ent, t, vert, tc);
}

void umtmapautotile(UmkaStackSlot *p, UmkaStackSlot *r) {
	uu tile = p[0].intVal;
	uu *cfg = (uu *)p[1].ptrVal;
	uu *src = (uu *)p[2].ptrVal;
	uu h = p[3].intVal;
	uu w = p[4].intVal;
	uu *tgt = (uu *)p[5].ptrVal;

	th_tmap_autotile(tgt, src, w, h, cfg, tile);
}

///////////////////////
// images
// loads an image at respath + path
void umimgload(UmkaStackSlot *p, UmkaStackSlot *r) {
	char *path = (char *)p[0].ptrVal;

	char pathcpy[1024];
	th_image *img = th_alloc_image();
	if (!img) return;
	img = th_load_image(conv_path(pathcpy, path));
	img->gltexture = th_gen_texture(img->data, img->dm, img->filter);

	r[0].intVal = thg.image_count;
}

// checks, if image is correctly loaded
void umimgvalid(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[0].uintVal);
	if (!img) return;
	if (img->data != NULL) {
		r->intVal = 1;
		return;
	}

	r->intVal = 0;
}

// flips image
void umimgflipv(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[1].uintVal);
	if (!img) return;

	img->flipv = p[0].intVal;
}

// flips image
void umimgfliph(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[1].uintVal);
	if (!img) return;

	img->fliph = p[0].intVal;
}

void umimggetdims(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[0].uintVal);
	if (!img) return;
	th_vf2 *out = (th_vf2 *)p[1].ptrVal;

	if (!img)
		return;

	*out = img->dm;
}

void umimgcrop(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[2].uintVal);
	if (!img) return;
	th_vf2 tl = *(th_vf2 *)&p[1];
	th_vf2 br = *(th_vf2 *)&p[0];

	img->crop = (th_rect){tl.x, tl.y, br.x, br.y};
}

// returns a pointer to an image from data
void umimgfromdata(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_vf2 dm = *(th_vf2 *)&p[0];
	uint32_t *data = (uint32_t *)p[1].ptrVal;

	th_image *img = th_get_image_err(p[2].uintVal);
	if (!img) return;
	th_image_from_data(img, data, dm);

	r->intVal = thg.image_count;
}

void umimgcopy(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img1 = th_get_image_err(p[0].intVal);
	if (!img1) return;

	th_image *img2 = th_alloc_image();
	if (!img2) return;
	*img2 = *img1;
	img2->data = calloc(sizeof(uint32_t), img2->dm.w * img2->dm.h);
	memcpy(img2->data, img1->data, sizeof(uint32_t) * img2->dm.w * img2->dm.h);

	r->intVal = thg.image_count;
}

void umimgsetfilter(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[1].intVal);
	if (!img) return;
	int filter = p[0].intVal;

	th_image_set_filter(img, filter);
}

///////////////////////
// input
// gets position of mouse cursor
void umgetmouse(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_vf2 *out = (th_vf2 *)p[0].ptrVal;
	out->x = thg.mouse.x / thg.scaling;
	out->y = thg.mouse.y / thg.scaling;
}

void umispressed(UmkaStackSlot *p, UmkaStackSlot *r) {
	int keycode = p[0].intVal;

	r[0].intVal = thg.pressed[keycode];
}

void umisjustpressed(UmkaStackSlot *p, UmkaStackSlot *r) {
	int keycode = p[0].intVal;

	r[0].intVal = thg.just_pressed[keycode];
}

///////////////////////
// entities
// draws an entity
void umentdraw(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_rect *rc = (th_rect *)p[0].ptrVal;
	th_ent *e = (th_ent *)p[1].ptrVal;
	th_ent_draw(e, rc);
}

void umentgetcoll(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_ent **scene = (th_ent **)p[0].ptrVal;
	th_ent *e = (th_ent *)p[1].ptrVal;
	int count = p[2].intVal;
	uu maxColls = p[3].intVal;
	uu *collC = (uu *)p[4].ptrVal;
	th_coll *colls = (th_coll *)p[5].ptrVal;

	th_ent_getcoll(e, scene, count, collC, maxColls, colls);
}

int _th_ysort_test(const void *a, const void *b) {
	return *((double *)a + 1) - *((double *)b + 1);
}

void umentysort(UmkaStackSlot *p, UmkaStackSlot *r) {
	void *ents = (void *)p[1].ptrVal;
	int count = p[0].intVal;

	qsort(ents, count, 140, _th_ysort_test);
}

///////////////////////
// audio
void umauload(UmkaStackSlot *p, UmkaStackSlot *r) {
	char path[1024];
	th_sound *s = th_sound_load(conv_path(path, (char *)p->ptrVal));
	if (!s) return;

	r->intVal = thg.sound_count;
}

// sets array of sounds to be played
void umsoundloop(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound_err(p[1].uintVal);
	if (!s) return;
	s->looping = p[0].intVal;
}

void umsoundplay(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound_err(p[0].uintVal);
	if (!s) return;
	s->playing = 1;
}

void umsoundpause(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound_err(p[0].uintVal);
	if (!s) return;
	s->playing = 0;
}

void umsoundvol(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound_err(p[1].uintVal);
	if (!s) return;
	s->volume = p[0].real32Val;
}

// checks, if sound is valid
void umsoundvalidate(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound(p[0].uintVal);
	if (!s) return;

	r[0].intVal = 1;
}

void umsoundstop(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_sound *s = th_get_sound_err(p[0].uintVal);
	if (!s) return;
	s->seek_to = 0;
	s->playing = 0;
}

///////////////////////
// raycast
void umraygetcoll(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_ent **scene = (th_ent **)p[0].ptrVal;
	th_ray *ra = (th_ray *)p[1].ptrVal;
	int count = p[2].intVal;
	th_vf2 *ic = (th_vf2 *)p[3].ptrVal;

	r->intVal = th_ray_getcoll(ra, scene, count, ic);
}

void umraygettmapcoll(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_ray *ra = (th_ray *)p[2].ptrVal;
	th_tmap *t = (th_tmap *)p[1].ptrVal;
	th_vf2 *ic = (th_vf2 *)p[0].ptrVal;

	r->intVal = th_ray_to_tilemap(ra, t, ic);
}

///////////////////////
// misc

// gets current time in ms
void umgettime(UmkaStackSlot *p, UmkaStackSlot *r) {
	struct timeval t;
	gettimeofday(&t, NULL);

	r->intVal = (long int)(t.tv_usec);
}

///////////////////////
// rawdraw TODO: hide rawdraw from c lib
// draws text
void umdrawtext(UmkaStackSlot *p, UmkaStackSlot *r) {
	fu size = p[0].real32Val;
	uint32_t color = (uint32_t)p[1].uintVal;
	th_vf2 pos = *(th_vf2 *)&p[2];
	char *text = (char *)p[3].ptrVal;

	th_canvas_text(text, color, pos, size);
}

void umwindowsetup(UmkaStackSlot *p, UmkaStackSlot *r) {
	char *title = (char *)p[2].ptrVal;
	int w = p[1].intVal;
	int h = p[0].intVal;

	th_window_setup(title, w, h);

	th_gl_init();
	th_image_init();
	th_canvas_init();
}


void umwindowclearframe(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_window_clear_frame();
}

void umwindowgetdimensions(UmkaStackSlot *p, UmkaStackSlot *r) {
	int *w = (int *)p[1].ptrVal;
	int *h = (int *)p[0].ptrVal;

	th_window_get_dimensions(w, h);
}

void umwindowswapbuffers(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_window_swap_buffers();
}

void umwindowhandle(UmkaStackSlot *p, UmkaStackSlot *r) {
	r->intVal = th_window_handle();
}

// calculates scaling
void umgetscaling(UmkaStackSlot *p, UmkaStackSlot *r) {
	int camh = p[0].intVal;
	int camw = p[1].intVal;
	int h = p[2].intVal;
	int w = p[3].intVal;

	if ((float)w/camw < (float)h/camh) {
		thg.scaling = ((float)w/camw);
	} else {
		thg.scaling = ((float)h/camh);
	}
}

void umcanvasrect(UmkaStackSlot *p, UmkaStackSlot *r) {
	uint32_t color = p[2].uintVal;
	th_rect re = *(th_rect *)&p[0];
	th_canvas_rect(color, re);
}

void umcanvasline(UmkaStackSlot *p, UmkaStackSlot *r) {
	float thickness = p[0].real32Val;
	th_vf2 e = *(th_vf2 *)&p[1];
	th_vf2 b = *(th_vf2 *)&p[2];
	uint32_t color = p[3].uintVal;

	th_canvas_line(color, b, e, thickness);
}

void umimagedraw(UmkaStackSlot *p, UmkaStackSlot *r) {
	th_image *img = th_get_image_err(p[2].uintVal);
	if (!img) return;
	th_transform *t = (th_transform *)p[1].ptrVal;
	uint32_t c = p[0].uintVal;

	th_blit_tex(img, *t, c);
}

void _th_umka_bind(void *umka) {
	// etc
	umkaAddFunc(umka, "cfopen", &umfopen);

	umkaAddFunc(umka, "ctexttoimg", &umfonttexttoimg);
	umkaAddFunc(umka, "cfontload", &umfontload);
	umkaAddFunc(umka, "getYOff", &umfontgetyoff);

	umkaAddFunc(umka, "clightmaskclear", &umlightmaskclear);
	umkaAddFunc(umka, "clightmaskdraw", &umlightmaskdraw);
	umkaAddFunc(umka, "cspotlightstamp", &umspotlightstamp);

	umkaAddFunc(umka, "c_particles_draw", &umparticlesdraw);

	// tilemaps
	umkaAddFunc(umka, "cdrawtmap", &umdrawtmap);
	umkaAddFunc(umka, "ctmapgetcoll", &umtmapgetcoll);
	umkaAddFunc(umka, "cautotile", &umtmapautotile);

	// images
	umkaAddFunc(umka, "loadimg", &umimgload);
	umkaAddFunc(umka, "flipvimg", &umimgflipv);
	umkaAddFunc(umka, "fliphimg", &umimgfliph);
	umkaAddFunc(umka, "imgvalid", &umimgvalid);
	umkaAddFunc(umka, "imggetdims", &umimggetdims);
	umkaAddFunc(umka, "imgcrop", &umimgcrop);
	umkaAddFunc(umka, "imgfromdata", &umimgfromdata);
	umkaAddFunc(umka, "imgcopy", &umimgcopy);
	umkaAddFunc(umka, "imgsetfilter", &umimgsetfilter);

	// input
	umkaAddFunc(umka, "cgetmouse", &umgetmouse);
	umkaAddFunc(umka, "cispressed", &umispressed);
	umkaAddFunc(umka, "cisjustpressed", &umisjustpressed);

	// entities
	umkaAddFunc(umka, "centdraw", &umentdraw);
	umkaAddFunc(umka, "cgetcoll", &umentgetcoll);
	umkaAddFunc(umka, "centysort", &umentysort);

	// rays
	umkaAddFunc(umka, "craygetcoll", &umraygetcoll);
	umkaAddFunc(umka, "craygettmapcoll", &umraygettmapcoll);

	// audio
	umkaAddFunc(umka, "cauload", &umauload);
	umkaAddFunc(umka, "csoundloop", &umsoundloop);
	umkaAddFunc(umka, "csoundplay", &umsoundplay);
	umkaAddFunc(umka, "csoundstop", &umsoundstop);
	umkaAddFunc(umka, "csoundpause", &umsoundpause);
	umkaAddFunc(umka, "csoundvol", &umsoundvol);
	umkaAddFunc(umka, "csoundvalidate", &umsoundvalidate);

	// misc
	umkaAddFunc(umka, "getTime", &umgettime);

	// canvas
	umkaAddFunc(umka, "drawText", &umdrawtext);
	umkaAddFunc(umka, "wsetup", &umwindowsetup);
	umkaAddFunc(umka, "clearframe", &umwindowclearframe);
	umkaAddFunc(umka, "getdimensions", &umwindowgetdimensions);
	umkaAddFunc(umka, "swapbuffers", &umwindowswapbuffers);
	umkaAddFunc(umka, "handleinput", &umwindowhandle);
	umkaAddFunc(umka, "updatescaling", &umgetscaling);
	umkaAddFunc(umka, "drawRect", &umcanvasrect);
	umkaAddFunc(umka, "drawLine", &umcanvasline);
	umkaAddFunc(umka, "cdrawimage", &umimagedraw);

	int index = 0;
	umkaAddModule(umka, "anim.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "audio.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "csv.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "ent.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "image.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "input.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "misc.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "canvas.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "ray.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "rect.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "tilemap.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "window.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "ui.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "std.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "particles.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "light.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "lerp.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "map.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "utf8.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "font.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "th.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "signal.um", th_em_modulesrc[index++]);
	umkaAddModule(umka, "atlas.um", th_em_modulesrc[index++]);
}
