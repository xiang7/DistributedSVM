import numpy as np
import scipy as sp
import scipy.sparse as sparse
from scipy.sparse import coo_matrix

f = open("test_data")
i = []
j = []
y = []
data = []
for ii, l in enumerate(f):
	l = l.split()
	y.append(l[0])
	for ll in l[1:]:
		[jj, val] = ll.split(":")
		i.append(int(ii))
		j.append(int(jj))
		data.append(float(val))

X = coo_matrix((data, (i,j))
