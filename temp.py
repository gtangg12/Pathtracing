fin = open('OBJ/pear.txt', 'r')
fout = open('OBJ/pear2.txt', 'w')
for line in fin.readlines():
    arr = line.split(' ')
    arr[-1] = arr[-1][:-1]
    if arr[0] == 'f':
        for i in range(1, len(arr)):
            arr2 = arr[i].split('/')
            arr2[2], arr2[1] = arr2[1], arr2[2]
            arr[i] = '/'.join(arr2)
        temp = ' '.join(arr)
        fout.write(temp+'\n')
    else:
        fout.write(line)
