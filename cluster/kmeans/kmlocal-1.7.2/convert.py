import sys

f = open(sys.argv[1])
cols=[]
test=[]
max_fea=0
row_count=0
for line in f:
	col=[]
	s=line.split()
	test.append(int(s[0]))
	for i in range(1,len(s)):
		ss=s[i].split(':')
		col.append(ss[0])
		if int(ss[0])>max_fea:
			max_fea=int(ss[0])
	cols.append(col)
	row_count+=1

train=[]
for i in range(0,row_count):
	tmp=["0" for j in range(0,max_fea+1)]
	for j in cols[i]:
		tmp[int(j)]="1"
	train.append(tmp)


fpos=open("pos",'w')
fneg=open("neg",'w')
for i in range(0,len(test)):
	if test[i]==1:
		fpos.write(" ".join(train[i])+"\n");
	else:
                fneg.write(" ".join(train[i])+"\n");
fpos.close()
fneg.close()
