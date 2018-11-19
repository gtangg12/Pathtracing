import numpy as np
import cv2 as cv

#CBOX: 47
#CAR: 17
#ROOM: 5
#SPONZA: 28

from keras.models import *
from keras.callbacks import ModelCheckpoint
from keras.layers import *
from keras.optimizers import *
from keras.metrics import *
from keras.utils import *
from keras import backend as K

kernel = np.zeros((5, 5, 3, 3))
kernel[0,0,:,:] = kernel[4,0,:,:] = kernel[0,4,:,:] = kernel[4,4,:,:] = 5
kernel[0,1,:,:] = kernel[0,3,:,:] = kernel[1,0,:,:] = kernel[1,4,:,:] = 3
kernel[3,0,:,:] = kernel[3,4,:,:] = kernel[4,1,:,:] = kernel[4,3,:,:] = 3
kernel[0,2,:,:] = kernel[2,0,:,:] = kernel[2,4,:,:] = kernel[4,2,:,:] = 0
kernel[1,1,:,:] = kernel[1,3,:,:] = kernel[3,1,:,:] = kernel[3,3,:,:] = -12
kernel[1,2,:,:] = kernel[2,1,:,:] = kernel[2,3,:,:] = kernel[3,2,:,:] = -23
kernel[2,2,:,:] = -40
kernel = K.variable(kernel)
#kernel = np.array([[0, 0, 1, 0, 0], [0, 1, 2, 1, 0], [1, 2, -16, 2, 1], [0, 1, 2, 1, 0], [0, 0, 1, 0, 0]])

def LG(arr):
    edge = K.conv2d(arr, kernel, strides=(1, 1), padding='same', data_format='channels_last')
    mn = K.min(edge)
    mx = K.max(edge)
    return (edge-mn)/(mx-mn)
    #return K.variable(np.array([cv.filter2D(img ,-1, kernel) for img in K.eval(arr)]))

def optxloss(y_true, y_pred):
    return 0.7*K.mean(K.square(y_pred-y_true))+0.3*K.mean(K.square(LG(y_pred)-LG(y_true)))

def PSNRLoss(y_true, y_pred):
    return -10.*K.log(K.mean(K.square(255.*(y_pred - y_true)))) / K.log(10.)

def SSIM(y_true, y_pred):
    ux = K.mean(y_pred)
    uy = K.mean(y_true)
    varx = K.var(y_pred)
    vary = K.var(y_true)
    c1 = 0.01**2
    c2 = 0.03**2
    ssim = (2*ux*uy+c1)*(2*K.sqrt(varx)*K.sqrt(vary)+c2)
    denom =(ux**2+uy**2+c1)*(varx+vary+c2)
    return ssim/K.clip(denom, K.epsilon(), np.inf)

def border(img, pos, color):
    temp = img.copy()
    w = 3
    cs = 128
    for i in range(pos[0], pos[0]+cs):
        for j in range(pos[1], pos[1]+cs):
            if i<pos[0]+w or i>pos[0]+cs-w-1 or j<pos[1]+w or j>pos[1]+cs-w-1:
                temp[i:i+1,j:j+1] = color
    return temp


folders = ['CBOX', 'CAR', 'ROOM', 'SPONZA']
names = ['cbox47', 'car17', 'room5', 'sponza288']
cs = 128
crops = [((256, 0), (56, 272)), ((256, 320), (280, 150)), ((366, 50), (50, 350)), ((306, 206), (380, 340))]

mixed = load_model('DMIXED/model-097.h5', custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
for i in range(0, 4):
    clean = np.array([cv.imread(folders[i]+'/'+folders[i]+'4096ssp/'+names[i]+'.jpg')])/255.
    noisy = np.array([cv.imread(folders[i]+'/'+folders[i]+'4ssp/'+names[i]+'.jpg')])/255.
    dif = np.array([cv.imread(folders[i]+'/'+folders[i]+'dif/'+names[i]+'.jpg')])/255.
    model = load_model('D'+folders[i]+'/final.h5', custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
    res = model.predict([np.concatenate((noisy, dif), axis = 3)])
    res2 = mixed.predict([np.concatenate((noisy, dif), axis = 3)])
    lb1 = crops[i][0]
    lb2 = crops[i][1]
    clean = clean[0]
    noisy = noisy[0]
    res = res[0]
    res2 = res2[0]
    blue = np.array([255, 0, 0])/255.
    yellow = np.array([0, 255, 255])/255.

    c1 = clean[lb1[0]:lb1[0]+cs,lb1[1]:lb1[1]+cs]
    c1 = border(c1, (0, 0), yellow)
    c2 = clean[lb2[0]:lb2[0]+cs,lb2[1]:lb2[1]+cs]
    c2 = border(c2, (0, 0), blue)

    c3 = noisy[lb1[0]:lb1[0]+cs,lb1[1]:lb1[1]+cs]
    c3 = border(c3, (0, 0), yellow)
    c4 = noisy[lb2[0]:lb2[0]+cs,lb2[1]:lb2[1]+cs]
    c4 = border(c4, (0, 0), blue)

    c5 = res[lb1[0]:lb1[0]+cs,lb1[1]:lb1[1]+cs]
    c5 = border(c5, (0, 0), yellow)
    res = border(res, lb1, yellow)
    c6 = res[lb2[0]:lb2[0]+cs,lb2[1]:lb2[1]+cs]
    c6 = border(c6, (0, 0), blue)
    res = border(res, lb2, blue)

    c7 = res2[lb1[0]:lb1[0]+cs,lb1[1]:lb1[1]+cs]
    c7 = border(c7, (0, 0), yellow)
    c8 = res2[lb2[0]:lb2[0]+cs,lb2[1]:lb2[1]+cs]
    c8 = border(c8, (0, 0), blue)

    cv.imwrite('DATA/c'+names[i]+'1.jpg', 255.*c1)
    cv.imwrite('DATA/c'+names[i]+'2.jpg', 255.*c2)
    cv.imwrite('DATA/c'+names[i]+'N1.jpg', 255.*c3)
    cv.imwrite('DATA/c'+names[i]+'N2.jpg', 255.*c4)
    cv.imwrite('DATA/c'+names[i]+'S1.jpg', 255.*c5)
    cv.imwrite('DATA/c'+names[i]+'S2.jpg', 255.*c6)
    cv.imwrite('DATA/c'+names[i]+'M1.jpg', 255.*c7)
    cv.imwrite('DATA/c'+names[i]+'M2.jpg', 255.*c8)

    #cv.imshow(names[i], cv.resize(clean, (256, 256)))
    #cv.imshow(names[i]+'N', cv.resize(noisy, (256, 256)))
    cv.imwrite('DATA/'+names[i]+'S.jpg', cv.resize(255.*res, (256, 256)))
    #cv.imshow(names[i]+'M', cv.resize(res2, (256, 256)))
