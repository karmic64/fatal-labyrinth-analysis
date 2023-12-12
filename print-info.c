#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>



#define ROM_SIZE 0x20000

uint8_t rom[ROM_SIZE];



#define ROM_TEXT_ITEM_PREFIX 0xd1d1
#define ROM_TEXT_ITEM_SUFFIX 0xd187
#define ROM_TEXT_ITEM_COLOR 0xd4b1
#define ROM_TEXT_ENEMY 0xdc0b
#define ROM_TEXT_RANK 0xfbd3

#define ROM_EXP_TBL 0x5318

#define ROM_SPAWN_TBL 0x9ea6

#define ROM_ENEMY_MISC_TBL 0x9ffa
#define ROM_ENEMY_ATTACK_TBL 0xa08e
#define ROM_ENEMY_HP_TBL 0xa126
#define ROM_ENEMY_XP_TBL 0xa170
#define ROM_ENEMY_STAT_TBL 0xa1ba
#define ROM_ENEMY_POWER_TBL 0xa526

#define ROM_ITEM_TBL 0xcc4c

#define ROM_ITEM_SPAWN_TBL 0xf4ee




char * get_string(char * s, int i)
{
	if (i < 0)
		return "";
	else if (i == 0)
		return s;
	else
	{
		while (i > 0)
		{
			char c = *s++;
			if (!c)
				i--;
		}
		return s;
	}
}



unsigned get16(uint8_t *p)
{
	return (p[0]<<8) | p[1];
}



void separator()
{
	putchar('\n');
	for (int i = 0; i < 79; i++)
		putchar('-');
	putchar('\n');
	putchar('\n');
}



int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		puts("usage: print-info romname");
		return EXIT_FAILURE;
	}
	
	/********* read rom *********/
	{
		FILE *f = fopen(argv[1],"rb");
		if (!f)
		{
			printf("can't open rom: %s\n",strerror(errno));
			return EXIT_FAILURE;
		}
		fread(rom,1,ROM_SIZE,f);
		fclose(f);
		
		separator();
	}
	
	
	/************** ranks ****************/
	{
		char *s = &rom[ROM_TEXT_RANK];
		puts("Rank name / XP");
		for (size_t i = 0; i < 17; i++)
		{
			unsigned x = i == 0 ? 0 : get16(rom+ROM_EXP_TBL+((i-1)*2));
			printf("%s / %u\n",s,x);
			s += strlen(s)+2;
		}
		
		separator();
	}
	
	
	
	/************** enemies ***************/
	{
		char *s = &rom[ROM_TEXT_ENEMY];
		puts("      Enemy name /   HP /   XP /   AR /   PW /  Atk / Spd / Hall  /  Floor(s)");
		for (size_t i = 0; i <= 0x24; i++)
		{
			unsigned hp = get16(rom + ROM_ENEMY_HP_TBL + (i*2));
			unsigned xp = get16(rom + ROM_ENEMY_XP_TBL + (i*2));
			unsigned ar = rom[ROM_ENEMY_STAT_TBL + (i*2) + 0];
			unsigned pw = rom[ROM_ENEMY_STAT_TBL + (i*2) + 1];
			unsigned attack = rom[ROM_ENEMY_POWER_TBL + i];
			unsigned speed = rom[ROM_ENEMY_MISC_TBL + (i*4) + 2];
			unsigned hall = (rom[ROM_ENEMY_MISC_TBL + (i*4) + 1] & 0x10) > 0;
			
			printf("%16s / %4u / %4u / %4u / %4u / %4u / %3u / %4u",s,hp,xp,ar,pw,attack,speed,hall);
			
			if (i < 0x22)
			{
				printf("  /  ");
				for (unsigned floor = 1; floor <= 30; floor++)
				{
					size_t fti = (floor - 1) & 0x7e;
					
					uint8_t * spawns = &rom[get16(rom + ROM_SPAWN_TBL + fti)];
					
					if (memchr(spawns,i,4))
						printf("%u ",floor);
				}
			}
			
			putchar('\n');
			
			printf("                ");
			uint8_t * attacks = &rom[ROM_ENEMY_ATTACK_TBL + (i*4)];
			for (unsigned j = 0; j < 4; j++)
			{
				printf(" / ");
				
				switch (j)
				{
					case 0:
					{
						switch (attacks[j])
						{
							case 0x80:
								printf("Normal attack");
								break;
							case 0x82:
								printf("Steal gold");
								break;
							case 0x84:
								printf("Steal items");
								break;
							case 0x86:
								printf("Steal food");
								break;
							default:
								printf("%02X",attacks[j]);
								break;
						}
						break;
					}
					case 1:
					{
						switch (attacks[j])
						{
							case 0x80:
								printf("Ranged attack");
								break;
							case 0x82:
								printf("Freeze");
								break;
							case 0x84:
								printf("Melt gear");
								break;
							default:
								printf("%02X",attacks[j]);
								break;
						}
						break;
					}
					case 2:
					{
						switch (attacks[j])
						{
							case 0x80:
								printf("Fire");
								break;
							case 0x82:
								printf("Blizzard");
								break;
							case 0x84:
								printf("Thunder");
								break;
							case 0x86:
								printf("Blind");
								break;
							case 0x88:
								printf("Steal PW");
								break;
							case 0x8a:
								printf("Teleport");
								break;
							default:
								printf("%02X",attacks[j]);
								break;
						}
						break;
					}
					case 3:
					{
						switch (attacks[j])
						{
							case 0x80:
								printf("Chaos");
								break;
							case 0x82:
								printf("Sleep");
								break;
							case 0x84:
								printf("Steal max HP");
								break;
							case 0x86:
								printf("Duplicate");
								break;
							default:
								printf("%02X",attacks[j]);
								break;
						}
						break;
					}
				}
				
			}
			
			putchar('\n');
			putchar('\n');
			
			s += strlen(s)+1;
		}
		
		separator();
	}
	
	
	
	/************* items ****************/
	{
		puts("Item name / Power  / Floors (Probability within set)");
		
		for (size_t i = 1; i <= 0x5b; i++)
		{
			/*if (i == 0x2f)
				i = 0x57;*/
			
			uint8_t * item = &rom[ROM_ITEM_TBL + ((i-1)*3)];
			
			char * n1 = get_string(&rom[ROM_TEXT_ITEM_PREFIX], i/*tem[0]*/-1);
			unsigned power = item[1];
			char * n2 = get_string(&rom[ROM_TEXT_ITEM_SUFFIX], (item[2]>>4)-1);
			
			printf("%s%s / %u  / ",n1,n2,power);
			
			for (unsigned set = 0; set < 0x10; set++)
			{
				uint8_t * p = &rom[get16(rom + ROM_ITEM_SPAWN_TBL + (set*2))];
				
				unsigned floor1 = 1;
				while (floor1 < 0x1e)
				{
					unsigned floor2 = p[0];
					unsigned c = p[1];
					p += 2;
					
					unsigned instances = 0;
					for (size_t x = 0; x < c; x++)
						if (p[x] == i)
							instances++;
						
					if (instances)
						printf("%u-%u(%u/%u) ",floor1,floor2,instances,c);
					
					p += c;
					floor1 = floor2+1;
				}
			}
			
			putchar('\n');
		}
		
		separator();
	}
	
	
	
	/******************* item spawn sets ****************/
	{
		for (unsigned set = 0; set < 0x10; set++)
		{
			printf("Set %u: \n",set);
			
			uint8_t * p = &rom[get16(rom + ROM_ITEM_SPAWN_TBL + (set*2))];
			
			unsigned floor1 = 1;
			while (floor1 < 0x1e)
			{
				unsigned floor2 = p[0];
				unsigned c = p[1];
				p += 2;
				
				printf("  %u-%u: ",floor1,floor2);
				
				uint8_t instances[0x100];
				memset(&instances,0,sizeof(instances));
				for (size_t x = 0; x < c; x++)
					instances[p[x]]++;
				
				for (unsigned i = 0; i < 0x100; i++)
				{
					if (instances[i])
					{
						if (i >= 1 && i <= 0x5b)
						{
							uint8_t * item = &rom[ROM_ITEM_TBL + ((i-1)*3)];
							
							char * n1 = get_string(&rom[ROM_TEXT_ITEM_PREFIX], i/*tem[0]*/-1);
							char * n2 = get_string(&rom[ROM_TEXT_ITEM_SUFFIX], (item[2]>>4)-1);
							
							printf("%s%s",n1,n2);
						}
						else
						{
							printf("$%02X",i);
						}
						
						printf(" (%u/%u), ",instances[i],c);
					}
				}
				
				
				p += c;
				floor1 = floor2+1;
				putchar('\n');
			}
			
			
			
			
			putchar('\n');
		}
		
		separator();
	}
	
	
}
