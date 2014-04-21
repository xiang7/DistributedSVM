import sys
from sklearn.ensemble import *
from sklearn.svm import *
import argparse
import numpy
from scipy import sparse
from scipy.sparse import *
import math
import random
import operator

class RedHerring:
	"""Implement the Red Herring Attack"""
	def __init__(self, datafile):
		"""@param datafile - the svm training set in libsvm format"""
		self._datafile=datafile
		[self._train,self._label]=libsvm_read(datafile)
		#go through each row of the data set, find average nnz of each row, avg_nnz
		CSR=self._train.tocsr()
		[m,n]=CSR.shape
		avg_nnz=0
		for i in range(0,m,2):
			avg_nnz+=CSR.getrow(i).getnnz()
		self._avg_nnz=avg_nnz/m*2

	def create_point(self, number):
		"""create number of malicious point"""
		result=[]
		#number iterations
		for i in range(0,number):
			temp=[]
			#if even, label 1 else label -1
			if i % 2 ==0:
				temp.append(str(1))
			else:
				temp.append(str(-1))
			fea=set()
			[m,n]=self._train.shape
			#random avg nnz benign features
			for j in range(0,self._avg_nnz):
				r=random.randint(0,n-2) #benign feature is from 0 to n-2, n-1 is the added fake feature
				if r not in fea:
					fea.add(r)
				else:
					j=j-1
			for f in sorted(list(fea)):
				temp.append(str(f)+":"+str(1))
			#add a max_fea at the end, and if iteration is even, label 1, else label -1
			temp.append(str(n-1)+":"+str(1))
			result.append(' '.join(temp))
		#return the data points
		return result
			
		

class Inseparability:
	"""Implement the Inseparability Attack"""
	def __init__(self, datafile):
		"""@param datafile - the svm training set in libsvm format"""
		self._datafile=datafile
		[self._train,self._label]=libsvm_read(datafile)
		#go through each row of the data set, find average nnz of each row, avg_nnz
		CSR=self._train.tocsr()
		[m,n]=CSR.shape
		avg_nnz=0
		for i in range(0,m,2):
			avg_nnz+=CSR.getrow(i).getnnz()
		self._avg_nnz=avg_nnz/m*2

	def create_point(self, number):
		"""create number of malicious point"""
		result=[]
		#number iterations
		for i in range(0,number):
			temp=[]
			#a random label for each point
			r=random.randint(0,1)
			if r==0:
				r=-1
			temp.append(str(r))
			fea=set()
			[m,n]=self._train.shape
			#random avg nnz benign features
			for j in range(0,self._avg_nnz):
				r=random.randint(0,n-2) #benign feature is from 0 to n-2, n-1 is the added fake feature
				if r not in fea:
					fea.add(r)
				else:
					j=j-1
			for f in sorted(list(fea)):
				temp.append(str(f)+":"+str(1))
			result.append(' '.join(temp))
		#return the data points
		return result

class FurthestFirstFlip:
	"""Implement the Furthest First Flip Attack"""

	def __init__(self, datafile):
		"""@param datafile - the svm training set in libsvm format"""
		self._datafile=datafile
		[train,self._label]=libsvm_read(datafile)
		self._data=train.data
		self._row=train.row
		self._col=train.col

	def create_point(self,number, stepsize):
		"""create number of malicious point, stepsize is the number of points chosen in each svm training, experiments should use the same stepsize as this to guarantee same performance"""
		#do the following for number/stepsize iterations
		result=[]
		for i in range(0,number/stepsize):
			print i
			#train svm
			train=coo_matrix((self._data,(self._row,self._col)))
			model=LinearSVC().fit(train,self._label)
			#for each data point in training, find distance to decision plane
			predict=model.decision_function(train)
			#sort according to absolute value of decision plane, get the top stepsize points, add to training set
			zipped=zip(predict,[k for k in range(0,len(predict))])
			sorted_zipped=sorted(zipped,key=lambda x: abs(x[0]),reverse=True)
			[m,n]=train.shape
			for j in range(0,stepsize):
				tmp=[]
				idx=sorted_zipped[j][1]
				#add to the train matrix
				x=train.getrow(idx).tocoo()
				t_data=x.data
				t_col=x.col
				self._data=numpy.append(self._data,t_data)
				self._col=numpy.append(self._col,t_col)
				self._row=numpy.append(self._row,[m for k in range(0,len(t_data))])
				self._label.append(-self._label[idx])
				#turn into string
				tmp.append(str(-self._label[idx]))
				for k in range(len(t_data)-1,-1,-1):
					tmp.append(str(t_col[k])+":"+str(t_data[k]))
				result.append(" ".join(tmp))
				m=m+1
		return result
		
	

def clean(file_name,outputfile):
	"""Remove the data points without features"""
	out=open(outputfile,'w')
        f=open(file_name,'r')
        for line in f:
                s=line.strip().split(' ')
		if len(s)<2:
			continue
		out.write(line.strip()+'\n')

def libsvm_read(file_name):
	"""read data set in libsvm format. return a coo_matrix"""
        col=[]
        row=[]
        data=[]
        label=[]
        row_count=0
	max_fea=0
        f=open(file_name,'r')
        for line in f:
                s=line.split(' ')
                label.append(float(s[0]))
                for i in range(1,len(s)):
                        ss=s[i].split(':')
                        col.append(int(ss[0]))
			if int(ss[0])>max_fea:
				max_fea=int(ss[0])
                        row.append(row_count)
                        data.append(float(ss[1]))
                row_count+=1
	train=coo_matrix((data,(row,col)),shape=(row_count,max_fea+2)) #add 1 additional feature to support red herring attack 
        return [train,label]

def rh_create(filename,number,outputfile):
	rh=RedHerring(filename)
	points=rh.create_point(number)
	export_list(points,outputfile)

def insep_create(filename,number,outputfile):
	rh=Inseparability(filename)
	points=rh.create_point(number)
	export_list(points,outputfile)

def fff_create(filename,number,stepsize,outputfile):
	fff=FurthestFirstFlip(filename)
	points=fff.create_point(number,stepsize)
	export_list(points,outputfile)

def export_list(l,file_name):
	with open(file_name,'w') as f:
		for item in l:
			f.write(item+'\n')

#clean a data set
#clean('w8a','w8aclean')
#create for rh attack
#rh_create('w8aclean',45546,'rh_w8aclean')
#create for inseparability attack
#insep_create('w8aclean',45546,'insep_w8aclean')
#create for fff attack
fff_create('test',46000,100,'fff_w8aclean')
