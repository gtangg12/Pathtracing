import numpy as np
import cv2 as cv
import glob

folders = ['CAR', 'CBOX', 'SPONZA', 'ROOM']
x = []
y = []
d = []
np.random.seed(1)
for folder in folders:
    noisy = [file for file in glob.glob(folder+'/c'+folder+'4ssp/*.jpg')]
    clean = [file for file in glob.glob(folder+'/c'+folder+'4096ssp/*.jpg')]
    diffm = [file for file in glob.glob(folder+'/c'+folder+'dif/*.jpg')]
    ind = np.arange(len(clean))
    np.random.shuffle(ind)
    for val in range(len(ind)):
        noisy[val], noisy[ind[val]] = noisy[ind[val]], noisy[val]
        clean[val], clean[ind[val]] = clean[ind[val]], clean[val]
        diffm[val], diffm[ind[val]] = diffm[ind[val]], diffm[val]
    for i in range(2560):
        x.append(noisy[i])
        y.append(clean[i])
        d.append(diffm[i])

ind = np.arange(len(x))
np.random.shuffle(ind)
for val in range(len(ind)):
    x[val], x[ind[val]] = x[ind[val]], x[val]
    y[val], y[ind[val]] = y[ind[val]], y[val]
    d[val], d[ind[val]] = d[ind[val]], d[val]

x = [cv.imread(file) for file in x]
y = [cv.imread(file) for file in y]
d = [cv.imread(file) for file in d]
temp = [x, y, d]

types = ['4ssp', '4096ssp', 'dif']
for i in range(3):
    cnt = 0
    for img in temp[i]:
        cv.imwrite('MIXED/cMIXED'+types[i]+'/'+str(cnt)+'.jpg', img)
        cnt+=1
