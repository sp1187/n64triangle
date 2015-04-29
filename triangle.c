#include <stdint.h>
#include <stdbool.h>
#include <libdragon.h>

static resolution_t res = RESOLUTION_320x240;
static bitdepth_t bit = DEPTH_32_BPP;

#define FIXED_11_2(i,f) ((((int16_t)i) << 2) | f)
#define FIXED_16_16(i,f) ((((int16_t)i) << 16) | f)

#define INTPART_11_2(x) (x >> 2)
#define INTPART_16_16(x) (x >> 16)

bool major = 1;
uint32_t yl = FIXED_11_2(150,0);
uint32_t ym = FIXED_11_2(50,0);
uint32_t yh = FIXED_11_2(50,0);
uint32_t xl = FIXED_16_16(150,0);
uint32_t dxldy = FIXED_16_16(-1,0);
uint32_t xh = FIXED_16_16(50,0);
uint32_t dxhdy = FIXED_16_16(0,0);
uint32_t xm = FIXED_16_16(50,0);
uint32_t dxmdy = FIXED_16_16(0,0);

enum curvar_t {
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

	((volatile uint32_t *)0xA4100000)[0] = ((uint32_t)data | 0xA0000000);
	((volatile uint32_t *)0xA4100000)[1] = ((uint32_t)data | 0xA0000000) + length;

	enable_interrupts();
}

uint32_t triangle_commands[8] __attribute__ ((aligned (8)));

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

		switch(curvar){
			case YL: graphics_draw_text(disp,20,20,"YL"); break;
			case YM: graphics_draw_text(disp,20,20,"YM"); break;
			case YH: graphics_draw_text(disp,20,20,"YH"); break;
			case XL: graphics_draw_text(disp,20,20,"XL"); break;
			case XM: graphics_draw_text(disp,20,20,"XM"); break;
			case XH: graphics_draw_text(disp,20,20,"XH"); break;
			case DXLDY: graphics_draw_text(disp,20,20,"DxLDy"); break; 
			case DXMDY: graphics_draw_text(disp,20,20,"DxMDy"); break; 
			case DXHDY: graphics_draw_text(disp,20,20,"DxHDy"); break;
			default: break;

		}

		rdp_sync(SYNC_PIPE);
		rdp_set_default_clipping();
		rdp_attach_display(disp);

		rdp_enable_blend_fill();
		rdp_set_blend_color(0xFFFFFFFF);

		rdp_send(triangle_commands,sizeof triangle_commands);
		rdp_sync(SYNC_PIPE);
		rdp_detach_display();

		//Y lines
		graphics_draw_line(disp,0,INTPART_11_2(yh),320,INTPART_11_2(yh),0xFFFF0000);
		graphics_draw_line(disp,0,INTPART_11_2(ym),320,INTPART_11_2(ym),0xFF800000);
		graphics_draw_line(disp,0,INTPART_11_2(yl),320,INTPART_11_2(yl),0xFF000000);

		//X lines
		graphics_draw_line(disp,INTPART_16_16(xh),0,INTPART_16_16(xh),240,0x00FFFF00);
		graphics_draw_line(disp,INTPART_16_16(xm),0,INTPART_16_16(xm),240,0x0080FF00);
		graphics_draw_line(disp,INTPART_16_16(xl),0,INTPART_16_16(xl),240,0x0000FF00);

		display_show(disp);

		controller_scan();
		struct controller_data keys = get_keys_down();

		if(keys.c[0].R) {
			curvar = (curvar + 1) % NUM_VARS;
		}
		else if(keys.c[0].L) {
			curvar = (curvar - 1 + NUM_VARS) % NUM_VARS;
		}
		else if(keys.c[0].right){
			switch(curvar){
				case YL: yl += 4; break;
				case YM: ym += 4; break;
				case YH: yh += 4; break;
				case XL: xl += 65536; break;
				case XM: xm += 65536; break;
				case XH: xh += 65536; break;
				case DXLDY: dxldy += 65536; break;
				case DXMDY: dxmdy += 65536; break;
				case DXHDY: dxhdy += 65536; break;
				default: break;

			}
		}
		else if(keys.c[0].left){
			switch(curvar){
				case YL: yl -= 4; break;
				case YM: ym -= 4; break;
				case YH: yh -= 4; break;
				case XL: xl -= 65536; break;
				case XM: xm -= 65536; break;
				case XH: xh -= 65536; break;
				case DXLDY: dxldy -= 65536; break;
				case DXMDY: dxmdy -= 65536; break;
				case DXHDY: dxhdy -= 65536; break;
				default: break;
			}
		}
	}
}
