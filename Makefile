# Ivan Bystrov
# 21 July 2020
#
# Gaussian Blur Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -lpng -lm -pthread
OBJDIR = objs
SRCDIR = srcs
HDRDIR = hdrs
OBJ = $(OBJDIR)/main.o $(OBJDIR)/process_png.o $(OBJDIR)/blur_cpu.o $(OBJDIR)/error.o $(OBJDIR)/blur_helpers.o
OUTPUT = blur

$(OUTPUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUTPUT) $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -I $(HDRDIR)

.PHONY: clean
clean:
	rm $(OBJDIR)/*.o $(OUTPUT)
