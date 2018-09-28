fin = open('roomcopy.txt', 'r')
fout = open('temp2.txt', 'w')
lines = fin.readlines()

found = False
for line in lines:
    if line[:-1] == 'BOUND' and not found:
        found = True
        continue
    if line[:-1] == 'BOUND' and found:
        break
    if found:
        fout.write(line)
