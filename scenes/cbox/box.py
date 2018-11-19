fin = open('cbox.txt', 'r')
fout = open('cbox2.txt', 'w')
for line in fin.readlines():
    temp = line[:-1].split(' ')
    if temp[0] == 'v':
        for i in range(3):
            temp[i+1] = round(float(temp[i+1])*100, 3)
            temp[i+1] = str(temp[i+1])
        fout.write(' '.join(temp)+'\n')
    else:
        fout.write(line)
