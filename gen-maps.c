#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <setjmp.h>

#include <png.h>



#define AMT_MAPS 0x1f
#define MAP_WIDTH 32
#define MAP_HEIGHT 32
#define TILE_WIDTH 3
#define TILE_HEIGHT 2
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

typedef uint8_t map_t[MAP_HEIGHT][MAP_WIDTH];

map_t map_a;
map_t map_b;

uint8_t src_maps[AMT_MAPS][MAP_HEIGHT][MAP_WIDTH/8];



#define ROM_SIZE 0x20000
#define ROM_BG_B_PATTERNS 0x47be
#define ROM_BG_A_PATTERNS 0x47fa
#define ROM_MAP_ROOM_TBL 0x10452
#define ROM_MAP_STAIRS_TBL 0x108be
#define ROM_MAP_PIT_TBL 0x108f8
#define ROM_MAP_ALARM_TBL 0x1090a
#define ROM_MAP_LAYOUTS 0x10918

uint8_t rom[ROM_SIZE];



#define VRAM_SIZE 0x10000
#define CRAM_SIZE 0x80
#define CHAR_CNT (VRAM_SIZE / 8 / 4)

/*uint8_t vram[VRAM_SIZE];*/
/*uint8_t cram[CRAM_SIZE];*/



#define IMAGE_WIDTH (MAP_WIDTH * TILE_WIDTH * CHAR_WIDTH)
#define IMAGE_HEIGHT (MAP_HEIGHT * TILE_HEIGHT * CHAR_HEIGHT)
#define PALETTE_COLORS (0x80/2)

typedef png_byte char_t[CHAR_HEIGHT][CHAR_WIDTH];

char_t chars[CHAR_CNT];

png_byte image[IMAGE_HEIGHT][IMAGE_WIDTH];
png_byte * image_rows[IMAGE_HEIGHT];
png_color palette[PALETTE_COLORS];



void png_err(png_structp png, png_const_charp msg)
{
	printf("libpng error: %s\n",msg);
}
void png_warn(png_structp png, png_const_charp msg)
{
	printf("libpng warning: %s\n",msg);
}





/*****************************************************************************/

int main(int argc, char * argv[])
{
	if (argc != 4)
	{
		puts("gen-maps srcrom vram cram");
		return EXIT_FAILURE;
	}
	const char * src_rom_name = argv[1];
	const char * src_vram_name = argv[2];
	const char * src_cram_name = argv[3];
	FILE *inf = NULL;
	FILE *of = NULL;
	
	
	/******************* read ROM *********************/
	inf = fopen(src_rom_name,"rb");
	if (!inf)
	{
		printf("can't open rom: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	fread(rom,1,ROM_SIZE,inf);
	fclose(inf);
	
	
	/***************** read chars from vram **************/
	inf = fopen(src_vram_name,"rb");
	if (!inf)
	{
		printf("can't open vram: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	for (size_t char_id = 0; char_id < CHAR_CNT; char_id++)
	{
		uint8_t vram_char[8][4];
		fread(vram_char,1,sizeof(vram_char),inf);
		
		for (unsigned y = 0; y < CHAR_HEIGHT; y++)
		{
			for (unsigned x = 0; x < CHAR_WIDTH; x++)
			{
				uint8_t b = vram_char[y][x/2];
				png_byte color = (x & 1) ? (b & 0x0f) : (b >> 4);
				chars[char_id][y][x] = color;
			}
		}
	}
	fclose(inf);
	
	
	/*************** read palette from cram *******************/
	inf = fopen(src_cram_name,"rb");
	if (!inf)
	{
		printf("can't open cram: %s\n",strerror(errno));
		return EXIT_FAILURE;
	}
	for (size_t color = 0; color < PALETTE_COLORS; color++)
	{
		uint8_t b1 = fgetc(inf);
		uint8_t b2 = fgetc(inf);
		palette[color].red = b2 << 4;
		palette[color].green = (b2 & 0xf0);
		palette[color].blue = b1 << 4;
	}
	fclose(inf);
	
	
	/******************** depack source maps *********************/
	{
		uint8_t buf[0x1000];
		memset(buf,0,0x1000);
		
		uint8_t * s = &rom[ROM_MAP_LAYOUTS];
		uint8_t * d = (uint8_t*)(src_maps);
		unsigned i = 0xfee;
		unsigned f = 0;
		unsigned fb = 0;
		unsigned bl = ((s[0]<<8) | s[1]) + 1;
		s += 2;
		while (bl)
		{
			if (fb == 0)
			{
				f = *s++;
				fb = 0x80;
				bl--;
			}
			
			if (f & fb)
			{
				*d++ = *s;
				buf[i++] = *s++;
				i &= 0xfff;
				bl--;
			}
			else
			{
				unsigned w = (s[0]<<8) | s[1];
				s += 2;
				
				unsigned si = w>>4;
				unsigned c = (w&0x0f)+3;
				while (c--)
				{
					*d++ = buf[si];
					buf[i++] = buf[si++];
					si &= 0xfff;
					i &= 0xfff;
				}
				bl -= 2;
			}
			
			fb >>= 1;
		}
	}
	
	
	/********************* init image rows **********************/
	for (size_t y = 0; y < IMAGE_HEIGHT; y++)
		image_rows[y] = image[y];
	
	
	
	/******************** per map loop ***************************/
	for (unsigned map_id = 0; map_id < AMT_MAPS; map_id++)
	{
		printf("doing map %u...\n",map_id);
		memset(map_a,0,sizeof(map_a));
		memset(map_b,0,sizeof(map_b));
		
		/******** expand src map to bg b *************/
		for (size_t y = 0; y < MAP_HEIGHT; y++)
		{
			for (size_t x = 0; x < MAP_WIDTH; x++)
			{
				map_b[y][x] = (src_maps[map_id][y][x/8] & (0x80 >> (x & 7))) ? 3 : 0;
			}
		}
		
		
		/******** set main floor tiles *****************/
		{
			uint8_t * pt = &rom[ROM_MAP_ROOM_TBL + (map_id * 2)];
			uint8_t * p = &rom[0x10000 | (pt[0] << 8) | (pt[1])];
			
			while ((*p) < 0x80)
			{
				unsigned xs = *p++;
				unsigned ys = *p++;
				unsigned width = *p++;
				unsigned height = *p++;
				
				unsigned xe = xs + width;
				unsigned ye = ys + height;
				
				for (size_t y = ys; y < ye; y++)
				{
					for (size_t x = xs; x < xe; x++)
					{
						if (map_b[y][x] == 3)
							map_b[y][x] = 4;
					}
				}
			}
		}
		
		
		/************ place stairs ************/
		if (map_id < 29)
		{
			uint8_t * stairs = &rom[ROM_MAP_STAIRS_TBL + map_id*2];
			unsigned map_index = (stairs[0] << 8) | stairs[1];
			
			if (map_b[0][map_index-1] != 4)
			{
				map_b[0][map_index] = 0x01;
				map_a[0][map_index-0x20] = 0x09;
			}
			else
			{
				map_b[0][map_index] = 0x02;
				map_a[0][map_index-0x20] = 0x0a;
			}
		}
		
		
		/************ create bg a *************/
		{
			/* address maps here as a linear array (to copy the original routine better) */
			uint8_t * a3 = (uint8_t*)map_a;
			uint8_t * a2 = (uint8_t*)map_b;
			
			/* do upper walls */
			for (size_t d6 = 0; d6 < 0x3e0; d6++)
			{
				if (!a2[d6] && a2[d6+0x20] >= 3)
					a3[d6] = 1;
			}
			
			/* do left walls */
			for (size_t d6 = 0; d6 < 0x3e0; d6++)
			{
				if (!a2[d6] && a2[d6+1] && !a3[d6])
				{
					if (a3[d6+1] != 9 && a2[d6+1] != 1)
					{
						a3[d6] = 8;
					}
				}
			}
			
			/* do shadows */
			/* (yes this is ugly direct 68000-conversion code) */
			for (size_t d6 = 0x20; d6 < 0x400; d6++)
			{
				unsigned d0;
				
				if (a3[d6]) goto f1d4;	
				if (a3[d6-0x20] != 1) goto f180;
				d0 = 2;
				if (a3[d6-0x21] == 1) goto f1c8;
				if (a3[d6-0x21] == 9) goto f1c8;
				d0++;
				if (a3[d6-1] == 1) goto f1c8;
				if (a3[d6-1] == 8) goto f1c8;
				if (a3[d6-1] == 9) goto f1c8;
				d0++;
				goto f1c8;
f180:
				if (a3[d6-1] == 1) goto f198;
				if (a3[d6-1] == 9) goto f198;
				if (a3[d6-1] != 8) goto f1ae;
f198:
				d0 = 5;
				if (a3[d6-0x21] == 1) goto f1aa;
				if (a3[d6-0x21] != 8) goto f1c8;
f1aa:
				d0++;
				goto f1c8;
f1ae:
				d0 = 7;
				if (a3[d6-0x21] == 1) goto f1c8;
				if (a3[d6-0x21] == 9) goto f1c8;
				if (a3[d6-0x21] != 8) goto f1d4;
f1c8:
				if (!a2[d6]) goto f1d4;
				a3[d6] = d0;
f1d4:
			;
			}
			
			/* Do Left Walls ～Top Walls編～ */
			for (size_t d6 = 0; d6 < 0x400; d6++)
			{
				if (!a3[d6] && a3[d6+1]==1 && !a2[d6])
				{
					a3[d6] = 8;
				}
			}
		}
		
		
		/******* place pit *********/
		if (map_id < 29 && !((map_id+1) % 3))
		{
			uint8_t * pit = &rom[ROM_MAP_PIT_TBL + ((((map_id+1) / 3) - 1) << 1)];
			unsigned map_index = (pit[0] << 8) | pit[1];
			map_a[0][map_index] = 0x0e;
		}
		
		
		/******* place alarm *********/
		if (map_id < 29 && !((map_id+1) % 4))
		{
			uint8_t * alarm = &rom[ROM_MAP_ALARM_TBL + ((((map_id+1) / 4) - 1) << 1)];
			unsigned map_index = (alarm[0] << 8) | alarm[1];
			map_a[0][map_index] = 0x0f;
		}
		
		
		
		/********* create image data from maps *********/
		for (size_t map_y = 0; map_y < MAP_HEIGHT; map_y++)
		{
			for (size_t map_x = 0; map_x < MAP_WIDTH; map_x++)
			{
				unsigned tile_id_a = map_a[map_y][map_x];
				unsigned tile_id_b = map_b[map_y][map_x];
				
				uint8_t * tile_pattern_a = &rom[ROM_BG_A_PATTERNS + (tile_id_a * 0x0c)];
				uint8_t * tile_pattern_b = &rom[ROM_BG_B_PATTERNS + (tile_id_b * 0x0c)];
				
				for (size_t tile_y = 0; tile_y < TILE_HEIGHT; tile_y++)
				{
					for (size_t tile_x = 0; tile_x < TILE_WIDTH; tile_x++)
					{
						size_t tile_pattern_index = ((tile_x * TILE_HEIGHT) + tile_y) * 2;
						
						unsigned name_a = (tile_pattern_a[tile_pattern_index] << 8)
							| tile_pattern_a[tile_pattern_index + 1];
						unsigned name_b = (tile_pattern_b[tile_pattern_index] << 8)
							| tile_pattern_b[tile_pattern_index + 1];
						
						unsigned char_id_a = name_a & 0x7ff;
						unsigned char_id_b = name_b & 0x7ff;
						
						unsigned hflip_a = name_a & 0x800;
						unsigned hflip_b = name_b & 0x800;
						
						unsigned vflip_a = name_a & 0x1000;
						unsigned vflip_b = name_b & 0x1000;
						
						unsigned palette_a = name_a & 0x6000;
						unsigned palette_b = name_b & 0x6000;
						
						unsigned palette_or_a = (palette_a >> 13) << 4;
						unsigned palette_or_b = (palette_b >> 13) << 4;
						
						for (size_t char_y = 0; char_y < CHAR_HEIGHT; char_y++)
						{
							for (size_t char_x = 0; char_x < CHAR_WIDTH; char_x++)
							{
								size_t image_x = map_x;
								image_x *= TILE_WIDTH;
								image_x += tile_x;
								image_x *= CHAR_WIDTH;
								image_x += char_x;
								size_t image_y = map_y;
								image_y *= TILE_HEIGHT;
								image_y += tile_y;
								image_y *= CHAR_HEIGHT;
								image_y += char_y;
								
								unsigned color_a =
									chars[char_id_a][char_y ^ (vflip_a ? 7 : 0)][char_x ^ (hflip_a ? 7 : 0)];
								unsigned color_b =
									chars[char_id_b][char_y ^ (vflip_b ? 7 : 0)][char_x ^ (hflip_b ? 7 : 0)];
								
								image[image_y][image_x] =
									color_a ? (color_a | palette_or_a) : (color_b | palette_or_b);
							}
						}
					}
				}
			}
		}
		
		
		
		/******** output png image ***********/
		png_structp png_ptr = NULL;
		png_infop info_ptr = NULL;
		
		char out_name_buf[0x10];
		sprintf(out_name_buf,"map-%02u.png",map_id);
		of = fopen(out_name_buf,"wb");
		if (!of)
		{
			printf("can't open outfile: %s\n",strerror(errno));
			goto next_map;
		}
		
		png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING,NULL,png_err,png_warn);
		if (!png_ptr)
		{
			puts("can't create png struct");
			goto next_map;
		}
		
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			puts("can't create png info struct");
			goto next_map;
		}
		
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			
		}
		else
		{
			png_init_io(png_ptr,of);
			png_set_IHDR(png_ptr,info_ptr,IMAGE_WIDTH,IMAGE_HEIGHT,
				8,PNG_COLOR_TYPE_PALETTE,PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
			png_set_PLTE(png_ptr,info_ptr,palette,PALETTE_COLORS);
			png_set_rows(png_ptr,info_ptr,image_rows);
			png_write_png(png_ptr,info_ptr,PNG_TRANSFORM_PACKING,NULL);
			png_write_end(png_ptr,info_ptr);
		}
		
next_map:
		png_destroy_write_struct(&png_ptr,&info_ptr);
		if (of) fclose(of);
		putchar('\n');
	}
	
	
	
}
