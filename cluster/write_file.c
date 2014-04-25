#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

void write_file (double ** mat, int r_size, int c_size, int * label) {
	FILE * fd = fopen("matrix", "w");
	int i, j;
	for (i = 0; i < r_size; r++) {
		fprintf(fd, lable[i]);
		for (j = 0; j < c_size; j++) {
			if (mat[i][j])
			  frpintf(fd, " %d:%f", j, mat[i][j]);
		}
		fprintf(fd, "\n");
	}
	fclose(fd);
}


