files = ['banana1.txt', 'banana2.txt']

for file in files:
    fin = open('OBJ/'+file, 'r')
    fout = open('OBJ2/'+file, 'w')
    for line in fin.readlines():
        line = line[:-1].split(' ')
        line = [t for t in line if t!='']
        if line[0] == 'v' or line[0] == 'vn':
            line[1], line[3] = line[3], line[1]
        fout.write(' '.join(line)+'\n')
