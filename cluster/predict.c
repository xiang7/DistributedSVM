#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "svm.h"
#include <mpi.h>

int print_null(const char *s,...) {return 0;}

static int (*info)(const char *fmt,...) = &printf;

struct svm_node *x;
int max_nr_attr = 64;

struct svm_model* model;
int predict_probability=0;

static char *line = NULL;
static int max_line_len;
double *target;

void evaluate (double *res, int size) {
	int i;
	int correct=0;
	int t_p=0, t_n=0, f_p=0, f_n=0;
	for (i=0; i<size; i++) {
		if (res[i] == 1) {
			if (target[i] == 1)
			  t_p++;
			else
			  f_p++;
		}
		else {
			if (target[i] == 1)
			  f_n++;
			else
			  t_n++;
		}
	}
	double accuracy = (double)correct/(double)size;
	printf("t_p   t_n   f_p   f_n\n");
	printf("%d   %d   %d   %d   \n", t_p, t_n, t_p, f_n);
}

static char* readline(FILE *input)
{
	int len;

	if(fgets(line,max_line_len,input) == NULL)
		return NULL;

	while(strrchr(line,'\n') == NULL)
	{
		max_line_len *= 2;
		line = (char *) realloc(line,max_line_len);
		len = (int) strlen(line);
		if(fgets(line+len,max_line_len-len,input) == NULL)
			break;
	}
	return line;
}

void exit_input_error(int line_num)
{
	fprintf(stderr,"Wrong input format at line %d\n", line_num);
	exit(1);
}

double * predict(FILE *input, int &size)
{
	int correct = 0;
	int total = 0;
	double error = 0;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

	int svm_type=svm_get_svm_type(model);
	int nr_class=svm_get_nr_class(model);
	double *prob_estimates=NULL;
	size = 0;
	int tmp;
	while ( (tmp=fgetc(input)) != EOF) {
				if (tmp == '\n')
				  size++;
	}
	rewind(input);
	double * res = (double *) malloc (size*sizeof(double));
	target = (double *) malloc (size*sizeof(double));


	max_line_len = 1024;
	line = (char *)malloc(max_line_len*sizeof(char));
	int j = 0;
	while(readline(input) != NULL)
	{
		int i = 0;
		double target_label, predict_label;
		char *idx, *val, *label, *endptr;
		int inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0

		label = strtok(line," \t\n");
		if(label == NULL) // empty line
			exit_input_error(total+1);

		target_label = strtod(label,&endptr);
		if(endptr == label || *endptr != '\0')
			exit_input_error(total+1);

		while(1)
		{
			if(i>=max_nr_attr-1)	// need one more for index = -1
			{
				max_nr_attr *= 2;
				x = (struct svm_node *) realloc(x,max_nr_attr*sizeof(struct svm_node));
			}

			idx = strtok(NULL,":");
			val = strtok(NULL," \t");

			if(val == NULL)
				break;
			errno = 0;
			x[i].index = (int) strtol(idx,&endptr,10);
			if(endptr == idx || errno != 0 || *endptr != '\0' || x[i].index <= inst_max_index)
				exit_input_error(total+1);
			else
				inst_max_index = x[i].index;

			errno = 0;
			x[i].value = strtod(val,&endptr);
			if(endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
				exit_input_error(total+1);

			++i;
		}
		x[i].index = -1;

		predict_label = svm_predict(model,x);
		res[j] = predict_label;
		target[j] = target_label;
		j++;
	}
	return res;
}

void exit_with_help()
{
	printf(
	"Usage: svm-predict [options] test_file model_file output_file\n"
	"options:\n"
	"-b probability_estimates: whether to predict probability estimates, 0 or 1 (default 0); for one-class SVM only 0 is supported\n"
	"-q : quiet mode (no outputs)\n"
	);
	exit(1);
}

int main(int argc, char **argv)
{
	FILE *input;
	int rank;
	int nproc;
	int size;
	double *res, *conclusion;
	double time;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	time=MPI_Wtime();
	input = fopen(argv[1],"r");
	if(input == NULL)
	{
		fprintf(stderr,"can't open input file %s\n",argv[1]);
		exit(1);
	}

	char * file_name = strdup (argv[2]);
	char *input_file = (char *)malloc(strlen(file_name)+3);
	sprintf(input_file, "%s.%d", file_name, rank);
	if((model=svm_load_model(input_file))==0)
	{
		fprintf(stderr,"can't open model file %s\n",argv[2]);
		exit(1);
	}

	x = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	
	res = predict(input, size);
	if (rank == 0) {
		conclusion = (double *) malloc (size*sizeof(double));
	}
	MPI_Reduce (res, conclusion, size, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0) {
		FILE * res_file = fopen("result", "w");
		int k;
		for (k = 0; k < size; k++) {
			if (conclusion[k] >= 0)
			  conclusion[k] = 1;
			else
			  conclusion[k] = -1;
			fprintf(res_file, "%f\n", conclusion[k]);
		}
		time=MPI_Wtime()-time;
		evaluate(conclusion, size);
		printf("Time=%19.15f\n", time);
	}
	svm_free_and_destroy_model(&model);
	free(x);
	free(line);
	fclose(input);
	MPI_Finalize();
	return 0;
}
