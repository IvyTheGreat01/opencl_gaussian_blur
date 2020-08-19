# Ivan Bystrov
# 21 July 2020
#
# Gaussian Blur Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -lpng -lm -pthread
OBJDIR = objs
SRCDIR = srcs
HDRDIR = hdrs
OBJ = $(OBJDIR)/main.o $(OBJDIR)/process_png.o $(OBJDIR)/blur_cpu.o $(OBJDIR)/error.o $(OBJDIR)/blur_helpers.o $(OBJDIR)/blur_gpu.o
OUTPUT = blur

ROCM = /opt/rocm/opencl/lib
ROCM_INCL = -I$(ROCM)/include
ROCM_LINK = -L$(ROCM)/lib

$(OUTPUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUTPUT) $(CFLAGS) -lOpenCL -L $(ROCM)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -I $(HDRDIR)

.PHONY: clean
clean:
	rm $(OBJDIR)/*.o $(OUTPUT)
