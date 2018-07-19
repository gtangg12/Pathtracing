from random import uniform
from random import randint

fout = open('data.txt', 'w');
N = 20
L = 3
fout.write(str(N+L+1)+'\n')
fout.write("S 0 80 0 15 0.8888888888888888 -100000 99995\n")
for i in range(N):
    fout.write('S '+str(randint(0, 255))+' '+str(randint(0, 255))+' '+str(randint(0, 255))
    +' '+str(uniform(0.05, 0.75))+' '+str(uniform(5, 15))+' '+str(uniform(-5, 5))+' '+str(uniform(-3, 3))+' '+str(uniform(0.75, 2.2))+'\n')
for i in range(L):
    fout.write('L '+str(uniform(5, 15))+' '+str(uniform(-5, 5))+' '+str(uniform(-3, 3))+' '+str(uniform(0.1, 0.3))+'\n')
