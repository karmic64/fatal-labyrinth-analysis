#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>


const wchar_t * charset_jp_punctuation =
	L"ー、。";

const wchar_t * charset_ascii =
	L" !\"#$%&©()*+,-./"
	L"0123456789"
	L":;<=>?'"
	L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	L"[\\]~_";
	
const wchar_t * charset_hiragana = 
	L"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをんぁぃぅぇぉっゃゅょ"
	L"がぎぐげござじずぜぞだぢづでどばびぶべぼ";
	
const wchar_t * charset_katakana =
	L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォッャュョ"
	L"ガギグゲゴザジズゼゾダヂヅデドバビブベボ"
	;



void do_charset(FILE *f, const wchar_t * p, unsigned id)
{
	size_t i = 0;
	while (p[i])
	{
		fwprintf(f, L"%02X=%lc\n",id,p[i]);
		id++;
		i++;
	}
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		puts("usage: gen-tbl outname");
		return EXIT_FAILURE;
	}
	
	if (!setlocale(LC_ALL,""))
	{
		puts("can't set locale");
		return EXIT_FAILURE;
	}
	
	FILE *f = fopen(argv[1],"w");
	if (!f)
	{
		printf("can't open outfile: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	do_charset(f, charset_jp_punctuation, 0x10);
	do_charset(f, charset_ascii, 0x20);
	do_charset(f, charset_hiragana, 0x60);
	do_charset(f, charset_katakana, 0xb0);
	
	fclose(f);
}
