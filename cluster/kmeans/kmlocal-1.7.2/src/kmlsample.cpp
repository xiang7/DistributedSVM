//----------------------------------------------------------------------
//	File:		kmlsample.cpp
//	Programmer:	David Mount
//	Last modified:	05/14/04
//	Description:	Sample program for kmeans
//----------------------------------------------------------------------
// Copyright (C) 2004-2005 David M. Mount and University of Maryland
// All Rights Reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at
// your option) any later version.  See the file Copyright.txt in the
// main directory.
// 
// The University of Maryland and the authors make no representations
// about the suitability or fitness of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//----------------------------------------------------------------------

#include <cstdlib>			// C standard includes
#include <iostream>			// C++ I/O
#include <string>			// C++ strings
#include "KMlocal.h"			// k-means algorithms
#include "string.h"

using namespace std;			// make std:: available

//----------------------------------------------------------------------
// kmlsample
//
// This is a simple sample program for the kmeans local search on each
// of the four methods.  After compiling, it can be run as follows.
// 
//   kmlsample [-d dim] [-k nctrs] [-max mpts] [-df data] [-s stages]
//
// where
//	dim		Dimension of the space (default = 2)
//	nctrs		The number of centers (default = 4)
//	mpts		Maximum number of data points (default = 1000)
//	data		File containing data points
//			(If omitted mpts points are randomly generated.)
//	stages		Number of stages to run (default = 100)
//
// Results are sent to the standard output.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//  Global entry points
//----------------------------------------------------------------------
void getArgs(int argc, char **argv);	// get command-line arguments

void printSummary(			// print final summary
    const KMlocal&	theAlg,		// the algorithm
    const KMdata&	dataPts,	// the points
    KMfilterCenters&	ctrs);		// the centers

bool readPt(				// read a point
    istream&		in,		// input stream
    KMpoint&		p);		// point (returned)

void printPt(				// print a point
    ostream&		out,		// output stream
    const KMpoint&	p);		// the point

//----------------------------------------------------------------------
//  Global parameters (some are set in getArgs())
//----------------------------------------------------------------------

int	k		= 4;		// number of centers
int	dim		= 2;		// dimension
int	maxPts		= 100;		// max number of data points
int	stages		= 100;		// number of stages
istream* dataIn		= NULL;		// input data stream
int label		=1;

//----------------------------------------------------------------------
//  Termination conditions
//	These are explained in the file KMterm.h and KMlocal.h.  Unless
//	you are into fine tuning, don't worry about changing these.
//----------------------------------------------------------------------
KMterm	term(100, 0, 0, 0,		// run for 100 stages
		0.10,			// min consec RDL
		0.10,			// min accum RDL
		3,			// max run stages
		0.50,			// init. prob. of acceptance
		10,			// temp. run length
		0.95);			// temp. reduction factor

//compute mean of data of length n
double mean(double* data, int n){
	int i=0;
	double sum=0.0;
	for(i=0;i<n;i++)
		sum+=data[i];
	return sum/n;
}


void write_file (double * vec, int len, int label,FILE *fd) {
        int j=0;
                fprintf(fd,"%d", label);
                for (j = 0; j < len; j++) {
                        if (abs(vec[j])>0.000001)
                          fprintf(fd, " %d:%f", j, vec[j]);
                }
                fprintf(fd, "\n");
        }


//compute std of data of length n
double standard_deviation(double* data, int n)
{
    double mean=0.0, sum_deviation=0.0;
    int i=0;
    for(i=0; i<n;i++)
    {
        mean+=data[i];
    }
    mean=mean/n;
    for(i=0; i<n;i++)
    sum_deviation+=(data[i]-mean)*(data[i]-mean);
    return sqrt(sum_deviation/n);           
}

//compute the distance between two points
double dist(double* x, double* y, int len)
{
	double sum=0.0;
	for(int i=0;i<len;i++)
	{
		double diff=x[i] - y[i];
		sum = sum + diff * diff;
	}
	return sqrt(sum);
}


//compute the avg dist of a point x to a lot of points y
double avg_dist(double* x,double** y, int m, int n){
	int i=0;
	double sum=0.0;
	for(i=0;i<m;i++)
		sum+=dist(x,y[i],n);
	return sum/(m-1);
		
}

//test points x, of dim (m,n), find points that're too far away > mean+3std
//result contains the index, count is the number of results
void prob_test(double** x, int m, int n, int* result, int *count){
	int i=0;
	double dis[m];
	for(i=0;i<m;i++)
		dis[i]=avg_dist(x[i],x,m,n);
/*
	for (i=0;i<m;i++){
		for(int j=0;j<n;j++){
			printf("%f ",x[i][j]);
		}
		printf("\n\n\n\n");
	}
*/		
	double avg=mean(dis,m);
	double std=standard_deviation(dis,m);
	cout << avg <<"\n";
	cout << std <<"\n";
	*count=0;
	for(i=0;i<m;i++){
		if(dis[i]>avg+std){
			result[*count]=i;
			(*count)++;
		}	
		cout<<dis[i]<<" "<<avg+std<<(*count)<<"\n";
	}
}

void prob_test_dists(double *x, int m,int* result, int *count){
	double avg=mean(x,m);
	double std=standard_deviation(x,m);
	cout << "avg: "<< avg <<"\n";
	cout << "std: "<< std <<"\n";
	*count=0;
	int i=0;
	for(i=0;i<m;i++){
		if(x[i]>avg+std){
			result[*count]=i;
			(*count)++;
		}	
		cout<<"point "<<x[i]<<" "<<avg+std<<(*count)<<"\n";
	}
}

//if num is in array a of length len
int inArray(int *a, int len, int num){
	int i=0;
	for(i=0;i<len;i++)
		if(num==a[i])
			return 1;
	return 0;
}

//----------------------------------------------------------------------
//  Main program
//----------------------------------------------------------------------
int main(int argc, char **argv)
{
    getArgs(argc, argv);			// read command-line arguments
    term.setAbsMaxTotStage(stages);		// set number of stages

    KMdata dataPts(dim, maxPts);		// allocate data storage
    int nPts = 0;				// actual number of points
    						// generate points
    if (dataIn != NULL) {			// read points from file
	while (nPts < maxPts && readPt(*dataIn, dataPts[nPts])) nPts++;
    }
    else {					// generate points randomly
	nPts = maxPts;
	kmClusGaussPts(dataPts.getPts(), nPts, dim, k);
    }

/*
    cout << "Data Points:\n";			// echo data points
    for (int i = 0; i < nPts; i++)
	printPt(cout, dataPts[i]);
*/

    dataPts.setNPts(nPts);			// set actual number of pts
    dataPts.buildKcTree();			// build filtering structure

    KMfilterCenters ctrs(k, dataPts);		// allocate centers

    						// run each of the algorithms
    cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
    KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
    ctrs = kmLloyds.execute();			// execute
    //printSummary(kmLloyds, dataPts, ctrs);	// print summary

  //  cout << "centers: "<< ctrs.getK()<<"\n";
    KMcenterArray ctrpts=ctrs.getCtrPts();
    int result[ctrs.getK()];
    int count=0;
    prob_test(ctrpts,ctrs.getK(),ctrs.getDim(),result,&count);
//    prob_test_dists(ctrs.getDists(),ctrs.getDim(),result,&count);
    cout << "anomaly: "<<count<<"\n";
    KMctrIdxArray closeCtr = new KMctrIdx[dataPts.getNPts()];
    double* sqDist = new double[dataPts.getNPts()];
    ctrs.getAssignments(closeCtr, sqDist);
    char *tmp_name = (char *) malloc (11); 
    sprintf(tmp_name, "output%s", (label==1?"_pos":"_neg"));
    FILE* fd=fopen(tmp_name,"w");
    cout<<"label "<<label<<"\n";
    cout<<"Number of points: "<<dataPts.getNPts()<<"\n";
    for (int i=0;i<nPts;i++)
	if(!inArray(result,count,closeCtr[i]))    
		write_file(dataPts[i],ctrs.getDim(),label,fd);
    fclose(fd);

/*    cout << "\nExecuting Clustering Algorithm: Swap\n";
    KMlocalSwap kmSwap(ctrs, term);		// Swap heuristic
    ctrs = kmSwap.execute();
    printSummary(kmSwap, dataPts, ctrs);

    cout << "\nExecuting Clustering Algorithm: EZ-Hybrid\n";
    KMlocalEZ_Hybrid kmEZ_Hybrid(ctrs, term);	// EZ-Hybrid heuristic
    ctrs = kmEZ_Hybrid.execute();
    printSummary(kmEZ_Hybrid, dataPts, ctrs);

    cout << "\nExecuting Clustering Algorithm: Hybrid\n";
    KMlocalHybrid kmHybrid(ctrs, term);		// Hybrid heuristic
    ctrs = kmHybrid.execute();
    printSummary(kmHybrid, dataPts, ctrs);
*/
    kmExit(0);
}



//----------------------------------------------------------------------
//  getArgs - get command line arguments
//----------------------------------------------------------------------

void getArgs(int argc, char **argv)
{
    static ifstream dataStream;			// data file stream
    static ifstream queryStream;		// query file stream

    if (argc <= 1) {				// no arguments
  	cerr << "Usage:\n\n"
        << "   kmlsample [-d dim] [-k nctrs] [-max mpts] [-df data] [-s stages]\n"
        << "\n"
        << " where\n"
        << "    dim             Dimension of the space (default = 2)\n"
        << "    nctrs           The number of centers (default = 4)\n"
        << "    mpts            Maximum number of data points (default = 1000)\n"
        << "    data            File containing data points\n"
	<< "                    (If omitted mpts points are randomly generated.)\n"
        << "    stages          Number of stages to run (default = 100)\n"
        << "\n"
        << " Results are sent to the standard output.\n"
	<< "\n"
	<< " The simplest way to run it is:\n"
	<< "    kmlsample -df data_pts\n"
	<< "  or\n"
	<< "    kmlsample -max 50\n";
	kmExit(0);
    }
    int i = 1;
    while (i < argc) {				// read arguments
	if (!strcmp(argv[i], "-d")) {		// -d option
	    dim = atoi(argv[++i]);
	}
	else if (!strcmp(argv[i], "-k")) {	// -k option
	    k = atoi(argv[++i]);
	}
	else if (!strcmp(argv[i], "-max")) {	// -max option
	    maxPts = atoi(argv[++i]);
	}
	else if (!strcmp(argv[i], "-df")) {	// -df option
	    dataStream.open(argv[++i], ios::in);
	    if (!dataStream) {
		cerr << "Cannot open data file\n";
		kmExit(1);
	    }
	    dataIn = &dataStream;
	}
	else if (!strcmp(argv[i], "-l")) {      // label option
            label = atoi(argv[++i]);
        }
	else if (!strcmp(argv[i], "-s")) {	// -s option
	    stages = atoi(argv[++i]);
	}
	else {					// illegal syntax
	    cerr << "Unrecognized option.\n";
	    kmExit(1);
	}
	i++;
    }
}

//----------------------------------------------------------------------
//  Reading/Printing utilities
//	readPt - read a point from input stream into data storage
//		at position i.  Returns false on error or EOF.
//	printPt - prints a points to output file
//----------------------------------------------------------------------
bool readPt(istream& in, KMpoint& p)
{
    for (int d = 0; d < dim; d++) {
	if(!(in >> p[d])) return false;
    }
    return true;
}

void printPt(ostream& out, const KMpoint& p)
{
    out << "" << p[0];
    for (int i = 1; i < dim; i++) {
	out << " " << p[i];
    }
    out << "\n";
}

//------------------------------------------------------------------------
//  Print summary of execution
//------------------------------------------------------------------------
void printSummary(
    const KMlocal&		theAlg,		// the algorithm
    const KMdata&		dataPts,	// the points
    KMfilterCenters&		ctrs)		// the centers
{
    cout << "Number of stages: " << theAlg.getTotalStages() << "\n";
    cout << "Average distortion: " <<
	         ctrs.getDist(false)/double(ctrs.getNPts()) << "\n";
    					// print final center points
    cout << "(Final Center Points:\n";
    ctrs.print();
    cout << ")\n";
    					// get/print final cluster assignments
    KMctrIdxArray closeCtr = new KMctrIdx[dataPts.getNPts()];
    double* sqDist = new double[dataPts.getNPts()];
    ctrs.getAssignments(closeCtr, sqDist);

    *kmOut	<< "(Cluster assignments:\n"
		<< "    Point  Center  Squared Dist\n"
		<< "    -----  ------  ------------\n";
    for (int i = 0; i < dataPts.getNPts(); i++) {
	*kmOut	<< "   " << setw(5) << i
		<< "   " << setw(5) << closeCtr[i]
		<< "   " << setw(10) << sqDist[i]
		<< "\n";
    }
    *kmOut << ")\n";
    delete [] closeCtr;
    delete [] sqDist;
}
