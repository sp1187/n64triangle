#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <libdragon.h>

static resolution_t res = RESOLUTION_320x240;
static bitdepth_t bit = DEPTH_32_BPP;

#define WIDTH 320
#define HEIGHT 240

#define FIXED_11_2(i,f) ((((int16_t)i) << 2) | f)
#define FIXED_16_16(i,f) ((((int16_t)i) << 16) | f)

#define INTPART_11_2(x) (x >> 2)
#define INTPART_16_16(x) (x >> 16)

#define TOFLOAT_11_2(x) (x / 4.0f)
#define TOFLOAT_16_16(x) (x / 65536.0f)

#define DPC_START_REG (*((volatile uint32_t *)0xA4100000))
#define DPC_END_REG (*((volatile uint32_t *)0xA4100004))

bool major = 1;
int32_t yl = FIXED_11_2(150,0);
int32_t ym = FIXED_11_2(50,0);
int32_t yh = FIXED_11_2(50,0);
int32_t xl = FIXED_16_16(150,0);
int32_t dxldy = FIXED_16_16(-1,0);
int32_t xh = FIXED_16_16(50,0);
int32_t dxhdy = FIXED_16_16(0,0);
int32_t xm = FIXED_16_16(50,0);
int32_t dxmdy = FIXED_16_16(0,0);

enum {
	YL,
	YM,
	YH,
	XL,
	XM,
	XH,
	DXLDY,
	DXMDY,
	DXHDY,
	NUM_VARS
} curvar = YL;

void rdp_send(uint32_t *data, int length){
	data_cache_hit_writeback_invalidate(data,length);

	disable_interrupts();

	DPC_START_REG = (uint32_t) data;
	DPC_END_REG = (uint32_t) data + length;

	enable_interrupts();
}

uint32_t triangle_commands[8] __attribute__ ((aligned (8)));

void graphics_printf(display_context_t disp, int x, int y, char *szFormat, ...){
	char szBuffer[64];

	va_list pArgs;
	va_start(pArgs, szFormat);
	vsnprintf(szBuffer, sizeof szBuffer, szFormat, pArgs);
	va_end(pArgs);

	graphics_draw_text(disp, x, y, szBuffer);
}

void get_dxline_coords(int32_t x, int32_t y, int32_t dxdy, int* x1, int* y1, int* x2, int* y2) {
	float k = TOFLOAT_16_16(dxdy);
	float x_top = TOFLOAT_16_16(x) - TOFLOAT_11_2(y) * k;
	float x_bottom = x_top + k*HEIGHT;
	if (x_top < 0) {
		*x1 = 0;
		*y1 = (*x1-x_top) / k;
	}
	else if (x_top >= WIDTH) {
		*x1 = WIDTH - 1;
		*y1 = (*x1-x_top) / k;
	}
	else {
		*x1 = x_top;
		*y1 = 0;
	}

	if (x_bottom < 0) {
		*x2 = 0;
		*y2 = (*x2-x_top) / k;
	}
	else if (x_bottom >= WIDTH) {
		*x2 = WIDTH - 1;
		*y2 = (*x2-x_top) / k;
	}
	else {
		*x2 = x_bottom;
		*y2 = HEIGHT - 1;
	}
}

int main(void){
	static display_context_t disp = 0;

	init_interrupts();

	display_init(res,bit,2,GAMMA_NONE,ANTIALIAS_RESAMPLE);
	controller_init();
	rdp_init();

	for(;;){
		while( !(disp = display_lock()) );

		triangle_commands[0] = 0x08000000 | (major << 23) | yl;
		triangle_commands[1] = (ym << 16) | yh;
		triangle_commands[2] = xl;
		triangle_commands[3] = dxldy;
		triangle_commands[4] = xh;
		triangle_commands[5] = dxhdy;
		triangle_commands[6] = xm;
		triangle_commands[7] = dxmdy;

		graphics_fill_screen(disp,0x000000FF);

		int x1, y1, x2, y2;

		//DxDy lines
		get_dxline_coords(xh, yh, dxhdy, &x1, &y1, &x2, &y2);
		graphics_draw_line(disp,x1,y1,x2,y2,0x80FF8000);

		get_dxline_coords(xm, yh, dxmdy, &x1, &y1, &x2, &y2);
		graphics_draw_line(disp,x1,y1,x2,y2,0x40FF4000);

		get_dxline_coords(xl, ym, dxldy, &x1, &y1, &x2, &y2);
		graphics_draw_line(disp,x1,y1,x2,y2,0x00FF0000);

		//Y lines
		graphics_draw_line(disp,0,INTPART_11_2(yh),WIDTH-1,INTPART_11_2(yh),0xFFFF0000);
		graphics_draw_line(disp,0,INTPART_11_2(ym),WIDTH-1,INTPART_11_2(ym),0xFF800000);
		graphics_draw_line(disp,0,INTPART_11_2(yl),WIDTH-1,INTPART_11_2(yl),0xFF000000);

		//X lines
		graphics_draw_line(disp,INTPART_16_16(xh),0,INTPART_16_16(xh),HEIGHT-1,0x00FFFF00);
		graphics_draw_line(disp,INTPART_16_16(xm),0,INTPART_16_16(xm),HEIGHT-1,0x0080FF00);
		graphics_draw_line(disp,INTPART_16_16(xl),0,INTPART_16_16(xl),HEIGHT-1,0x0000FF00);

		switch(curvar){
			case YL: graphics_printf(disp,20,20,"YL %d", INTPART_11_2(yl)); break;
			case YM: graphics_printf(disp,20,20,"YM %d", INTPART_11_2(ym)); break;
			case YH: graphics_printf(disp,20,20,"YH %d", INTPART_11_2(yh)); break;
			case XL: graphics_printf(disp,20,20,"XL %d", INTPART_16_16(xl)); break;
			case XM: graphics_printf(disp,20,20,"XM %d", INTPART_16_16(xm)); break;
			case XH: graphics_printf(disp,20,20,"XH %d", INTPART_16_16(xh)); break;
			case DXLDY: graphics_printf(disp,20,20,"DxLDy %.3f", TOFLOAT_16_16(dxldy)); break;
			case DXMDY: graphics_printf(disp,20,20,"DxMDy %.3f", TOFLOAT_16_16(dxmdy)); break;
			case DXHDY: graphics_printf(disp,20,20,"DxHDy %.3f", TOFLOAT_16_16(dxhdy)); break;
			default: break;
		}

		graphics_printf(disp, 260, 20, major ? "Right" : "Left");

		rdp_sync(SYNC_PIPE);
		rdp_set_default_clipping();
		rdp_attach_display(disp);

		rdp_enable_blend_fill();
		rdp_set_blend_color(0xFFFFFFFF);

		rdp_send(triangle_commands,sizeof triangle_commands);
		rdp_sync(SYNC_PIPE);
		rdp_detach_display();

		display_show(disp);

		controller_scan();
		struct controller_data keys = get_keys_down();

		if(keys.c[0].R) {
			curvar = (curvar + 1) % NUM_VARS;
		}
		else if(keys.c[0].L) {
			curvar = (curvar - 1 + NUM_VARS) % NUM_VARS;
		}
		else if(keys.c[0].A) {
			major = !major;
		}
		else if(keys.c[0].right){
			switch(curvar){
				case YL: yl += FIXED_11_2(1,0); break;
				case YM: ym += FIXED_11_2(1,0); break;
				case YH: yh += FIXED_11_2(1,0); break;
				case XL: xl += FIXED_16_16(1,0); break;
				case XM: xm += FIXED_16_16(1,0); break;
				case XH: xh += FIXED_16_16(1,0); break;
				case DXLDY: dxldy += FIXED_16_16(0,8192); break;
				case DXMDY: dxmdy += FIXED_16_16(0,8192); break;
				case DXHDY: dxhdy += FIXED_16_16(0,8192); break;
				default: break;

			}
		}
		else if(keys.c[0].left){
			switch(curvar){
				case YL: yl -= FIXED_11_2(1,0); break;
				case YM: ym -= FIXED_11_2(1,0); break;
				case YH: yh -= FIXED_11_2(1,0); break;
				case XL: xl -= FIXED_16_16(1,0); break;
				case XM: xm -= FIXED_16_16(1,0); break;
				case XH: xh -= FIXED_16_16(1,0); break;
				case DXLDY: dxldy -= FIXED_16_16(0,8192); break;
				case DXMDY: dxmdy -= FIXED_16_16(0,8192); break;
				case DXHDY: dxhdy -= FIXED_16_16(0,8192); break;
				default: break;
			}
		}
	}
}
