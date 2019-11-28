import cv2 as cv
import glob
from random import randint

crops = 32
crop_size = 128
size = 512

def cropData(path1, path2, path3, path4):
    l1 = []
    l2 = []
    nrm = []
    dif = []
    for file in glob.glob(path1+'/*.jpg'):
        l1.append(cv.imread(file))
    for file in glob.glob(path2+'/*.jpg'):
        l2.append(cv.imread(file))
    for file in glob.glob(path3+'/*.jpg'):
        nrm.append(cv.imread(file))
    for file in glob.glob(path4+'/*.jpg'):
        dif.append(cv.imread(file))
    cnt = 0
    for f in range(len(l1)):
        for i in range(crops):
            r = randint(0, size-crop_size)
            c = randint(0, size-crop_size)
            img1 = l1[f][r:r+crop_size,c:c+crop_size]
            img2 = l2[f][r:r+crop_size,c:c+crop_size]
            img3 = nrm[f][r:r+crop_size,c:c+crop_size]
            img4 = dif[f][r:r+crop_size,c:c+crop_size]
            cv.imwrite('c'+path1+'/'+str(cnt)+'.jpg', img1)
            cv.imwrite('c'+path2+'/'+str(cnt)+'.jpg', img2)
            cv.imwrite('c'+path3+'/'+str(cnt)+'.jpg', img3)
            cv.imwrite('c'+path4+'/'+str(cnt)+'.jpg', img4)
            cnt+=1

cropData('CAR4ssp', 'CAR4096ssp', 'CARnrm', 'CARdif')
