fin = open('room.txt', 'r')
fout = open('temp.txt', 'w')
lines = fin.readlines()
fout2 = open('room.txt', 'w')

found = False
for line in lines:
    if line[0] == 'f':
        found = True;
    if line[0] == 'v' and found:
        fout = fout2
    fout.write(line)
