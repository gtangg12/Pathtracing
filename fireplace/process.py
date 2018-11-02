fin = open('fireplace.txt', 'r')
fout = open('fireplace2.txt', 'w')
input = fin.readlines()
for line in input:
    arr = line[:-1].split(' ')
    if arr[0] in ['v', 'vn', 'vt', 'f']:
        fout.write(line)
    elif arr[0] == 'o':
        #print('# '+' '.join(arr[1:]))
        fout.write('# '+' '.join(arr[1:])+'\n')
    elif arr[0] == 'u':
        #print('g '+' '.join(arr[1:]))
        fout.write('g '+' '.join(arr[1:])+'\n')
