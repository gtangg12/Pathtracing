fin = open('pine.txt', 'r')
fout = open('pine2.txt', 'w')
for line in fin.readlines():
    if (line[0]=='v' and line[1]!='n' and line[1]!='t'):
        fout.write('v'+line[2:])
    else:
        fout.write(line)
