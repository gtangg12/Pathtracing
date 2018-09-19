fin = open('room.txt', 'r')
fout = open('room2.txt', 'w')
good = set(['v', 'vn', 'vt', 'f', 'g', '#'])
lines = fin.readlines()
for i in range(0, len(lines)):
    if lines[i][0] in good and not (lines[i][0]=='g' and lines[i+1][0]=='v'):
        fout.write(lines[i])
