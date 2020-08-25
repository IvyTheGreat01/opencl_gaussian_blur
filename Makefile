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

ROCM = /opt/rocm/opencl
ROCM_INC = $(ROCM)/include
ROCM_LINK = $(ROCM)/lib

$(OUTPUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUTPUT) $(CFLAGS) -lOpenCL -L $(ROCM_LINK)

$(OBJDIR)/blur_gpu.o: $(SRCDIR)/blur_gpu.c $(HDRDIR)/blur_gpu.h $(SRCDIR)/kernels.cl
	$(CC) -c $< -o $@ $(CFLAGS) -I $(HDRDIR) -I $(ROCM_INC)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRDIR)/%.h
	$(CC) -c $< -o $@ $(CFLAGS) -I $(HDRDIR)

.PHONY: clean
clean:
	rm $(OBJDIR)/*.o $(OUTPUT)
