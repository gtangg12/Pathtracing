import numpy as np

def unit(v):
    mag = (v[0]**2+v[1]**2+v[2]**2)**0.5
    return v/mag

fin = open('OBJ/Reference/bunny.txt', 'r')
fout = open('OBJ/Reference/bunny2.txt', 'w')
vert = []
faces = []
norm = []
dict = {}
ind = 0
for line in fin.readlines():
    temp = line[:-1].split(' ')
    if (temp[0] != 'v' and temp[0] != 'f'):
        continue
    if temp[0] == 'v':
        for i in range(1, 4):
            temp[i] = float(temp[i])
        vert.append( np.array( [temp[1], temp[2], temp[3]] ) )
        fout.write(line)
    elif temp[0] == 'f':
        for i in range(1, 4):
            temp[i] = int(temp[i])-1
            if temp[i] not in dict:
                dict[temp[i]] = []
            dict[temp[i]].append(ind)
        faces.append( np.array( [temp[1], temp[2], temp[3]] ) )
        ca = vert[temp[3]] - vert[temp[1]]
        ba = vert[temp[2]] - vert[temp[1]]
        norm.append(np.cross(ba, ca))
        ind+=1
ans = []
vert_norm = []
for i, v in enumerate(vert):
    n = np.array([0.0, 0.0, 0.0])
    for j in dict[i]:
        n += norm[j]
    vert_norm.append(unit(n))
for v in vert_norm:
    fout.write('vn '+str(v[0])+' '+str(v[1])+' '+str(v[2])+'\n')
for f in faces:
    fout.write('f '+str(f[0]+1)+'/'+str(f[0]+1)+' '+str(f[1]+1)+'/'+str(f[1]+1)+' '+str(f[2]+1)+'/'+str(f[2]+1)+'\n')
