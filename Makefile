ifdef COMSPEC
DOTEXE:=.exe
else
DOTEXE:=
endif


CFLAGS:=-s -Ofast -Wall -Wextra
CLIBS:=-lpng


ROM:=FatalLabyrinth.bin

INGAME_VRAM:=ingame-vram.bin
INGAME_CRAM:=ingame-cram.bin

GEN_TBL_SRC:=./gen-tbl.c
GEN_TBL_EXE:=./gen-tbl$(DOTEXE)
TEXT_TBL:=text.tbl

GEN_MAPS_SRC:=./gen-maps.c
GEN_MAPS_EXE:=./gen-maps$(DOTEXE)

PRINT_INFO_SRC:=./print-info.c
PRINT_INFO_EXE:=./print-info$(DOTEXE)



.PHONY: all gen-maps print-info
all: $(TEXT_TBL) gen-maps


$(TEXT_TBL): $(GEN_TBL_EXE)
	$(GEN_TBL_EXE) $(TEXT_TBL)


gen-maps: $(GEN_MAPS_EXE)
	$(GEN_MAPS_EXE) $(ROM) $(INGAME_VRAM) $(INGAME_CRAM)


print-info: $(PRINT_INFO_EXE)
	$(PRINT_INFO_EXE) $(ROM) > INFO.txt



%$(DOTEXE): %.c
	$(CC) $(CFLAGS) -o $@ $< $(CLIBS)

