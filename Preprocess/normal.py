import numpy as np

def unit(v):
    mag = (v[0]**2+v[1]**2+v[2]**2)**0.5
    if (mag < 0.0000001):
        print (v, mag)
    return v/mag

fin = open('sheet.txt', 'r')
fout = open('../OBJ3/sheet1.txt', 'w')
vert = []
text = []
faces = []
norm = []
dict = {}
ind = 0
for line in fin.readlines():
    temp = line[:-1].split(' ')
    if (temp[0] != 'v' and temp[0] != 'f' and temp[0] != 'vt'):
        continue
    #vertex
    if temp[0] == 'v':
        for i in range(1, 4):
            temp[i] = float(temp[i])
        vert.append( np.array( [temp[1], temp[2], temp[3]] ) )
        fout.write(line)
    #face
    elif temp[0] == 'f':
        for i in range(1, len(temp)):
            temp[i] = [int(j) for j in temp[i].split('/')]
        for i in range(1, len(temp)):
            temp[i][0] = int(temp[i][0])-1
            if temp[i][0] not in dict:
                dict[temp[i][0]] = []
            dict[temp[i][0]].append(ind)
        ca = vert[temp[3][0]] - vert[temp[1][0]]
        ba = vert[temp[2][0]] - vert[temp[1][0]]
        val = np.cross(ba, ca)
        if (val[0] == 0.0 and val[1] == 0.0 and val[2] == 0.0 and len(temp)>4):
            ca = vert[temp[4][0]] - vert[temp[1][0]]
            ba = vert[temp[3][0]] - vert[temp[1][0]]
            val = np.cross(ba, ca)
        elif (val[0] == 0.0 and val[1] == 0.0 and val[2] == 0.0):
            print(vert[temp[1][0]], vert[temp[2][0]], vert[temp[3][0]])
        if (val[0] != 0.0 and val[1] != 0.0 and val[2] != 0.0):
            norm.append(val)
            ind+=1
            faces.append(temp)
    #texture
    else:
        fout.write(line)
ans = []
vert_norm = []
for i, v in enumerate(vert):
    n = np.array([0.0, 0.0, 0.0])
    for j in dict[i]:
        if (j<len(norm)):
            n += norm[j]
    vert_norm.append(unit(n))
for v in vert_norm:
    fout.write('vn '+str(v[0])+' '+str(v[1])+' '+str(v[2])+'\n')
for f in faces:
    for i in range(1, len(f)):
        f[i][1] = f[i][0]
        f[i] = '/'.join([str(t+1) for t in f[i]])
    fout.write(' '.join(f)+'\n')
