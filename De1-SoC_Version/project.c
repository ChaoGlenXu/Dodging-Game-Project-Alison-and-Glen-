#include "address_map_arm.h"
#include <stdbool.h>
#include <stdlib.h>
	
#define MAX_X 320
#define MAX_Y 240
#define MAX_RECTANGLES 8
	
void clear_screen();
void draw_line(int, int, int, int, short);
void draw_box(int, int, short);
void swap(int*, int*);
void plot_pixel(int, int, short);
void waitForVSync();
	
volatile int pixel_buffer_start; // global variable

int main(void)
{
	
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
	
	short colors[20] = {0xF800, 0xFC80, 0xFE60, 0xFFC0, 0xDFE0,
						0x97E0, 0x1FE0, 0x2628, 0x07F7, 0x07FF,
					    0x073F, 0x063F, 0x04DF, 0x03BF, 0x22B9,
					    0x101F, 0xAB5F, 0xA81F, 0xE01F, 0xF81C};
	
	short color_box[MAX_RECTANGLES],
		dx_box[MAX_RECTANGLES],
		dy_box[MAX_RECTANGLES],
		x_box[MAX_RECTANGLES],
		y_box[MAX_RECTANGLES];
	
	for (int i = 0; i < MAX_RECTANGLES; i++) {
		color_box[i] = colors[rand() % 20];
		dx_box[i] = (rand() % 2) * 2 + 1;
		dy_box[i] = (rand() % 2) * 2 + 1;
		x_box[i] = rand() % MAX_X;
		y_box[i] = rand() % MAX_Y;
	}
	
	
	*(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE;		//Set front buffer to onchip
	pixel_buffer_start = *pixel_ctrl_ptr;		// Need to take buffer value to clear screen
	clear_screen();								// Empty screen here as well, it looks cleaner at start
	waitForVSync();
	pixel_buffer_start = *pixel_ctrl_ptr;		// Need to take buffer value to clear screen
	clear_screen();								// Clear screen in first
	*(pixel_ctrl_ptr + 1) = SDRAM_BASE;			// Set back buffer to sdram

    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;
	
	
	while (true) {
		clear_screen();
		
		for (int i = 0; i < MAX_RECTANGLES - 1; i++) {
			draw_line(x_box[i], y_box[i], x_box[i+1], y_box[i+1], color_box[i]);
		}
		
		for (int i = 0; i < MAX_RECTANGLES; i++) {
			draw_box(x_box[i], y_box[i], color_box[i]);
			
			x_box[i] += dx_box[i];
			y_box[i] += dy_box[i];
			
			if (x_box[i] <= 0)
				dx_box[i] = 1;
			else if (x_box[i] > MAX_X - 4)		// The four is from box width/height
				dx_box[i] = -1;

			if (y_box[i] <= 0)
				dy_box[i] = 1;
			else if (y_box[i] > MAX_Y - 4)
				dy_box[i] = -1;
		}
		
		waitForVSync();
		pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	}
}

void swap(int* x, int* y) {
	int temp = *x;
	*x = *y;
	*y = temp;
}
	
void plot_pixel(int x, int y, short line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_screen() {
	for (unsigned short x = 0; x < MAX_X; x++) {
		for (unsigned short y = 0; y < MAX_Y; y++) {
			plot_pixel(x, y, 0xFFFF);	// this is white
		}
	}
}

void draw_line(int x0, int y0, int x1, int y1, short color) {
	bool isSteep = abs(y1 - y0) > abs (x1 - x0);
	
	if (isSteep) {		// Inverses the slope to make the for loop below work
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	if (x0 > x1) {		// Reverses order if x0 is ahead of x1 (backwards line)
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	
	short deltaX = x1 - x0;
	short deltaY = abs(y1 - y0);
	short error = -(deltaX)/2;
	short y = y0;
	short yStep = -1;	// By default, assuming the line goes upwards (from left to right)
	
	if (y0 < y1)		// If the line goes downwards,
		yStep = 1;		// Go down the screen instead of up the screen
	
	for (short x = x0; x <= x1; x++) {
		if (isSteep)
			plot_pixel(y, x, color);		// Changed x and y, because we swapped it earlier. 
		else
			plot_pixel(x, y, color);
		
		error = error + deltaY;
		if (error >= 0) {
			y += yStep;
			error -= deltaX;
		}
	}
}

void draw_box(int x, int y, short color) {
	for (unsigned short i = 0; i < 4; i++) {
		for (unsigned short j = 0; j < 4; j++)
			plot_pixel(x + i, y + j, color);
	}
}

void waitForVSync() {
	volatile int* pixel_ctrl_ptr = (int *) 0xFF203020;	//pixel controller 
	register int status;
	
	*pixel_ctrl_ptr = 1;	//start the synchronization procress
	
	status = *(pixel_ctrl_ptr + 3);
	
	while ((status & 0x01) != 0) {
		status = *(pixel_ctrl_ptr + 3);
	}
}