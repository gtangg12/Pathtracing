import numpy as np
import cv2 as cv
import glob
from random import randint

def getData(N, M):
    cnt = 0
    data = []
    names = []
    for file in glob.glob('val2014/*.jpg'):
        if cnt >= N:
            break
        img = cv.imread(file)
        height, width = img.shape[:2]
        if height<400 or width<400:
            continue
        cx = randint(0, height-M)
        cy = randint(0, width-M)
        data.append(img[cx:cx+M,cy:cy+M])
        names.append(file.split('/')[1])
        cnt+=1
    return np.array(data), np.array(names)

N = 20000
data, names = getData(N, 256)
for i in range(N):
    cv.imwrite('train/'+names[i], data[i])
