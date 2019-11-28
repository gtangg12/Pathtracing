import cv2 as cv
import glob
from keras.models import *
from keras.callbacks import ModelCheckpoint
from keras.layers import *
from keras.layers.advanced_activations import *
from keras.optimizers import *
from keras.metrics import *
from keras.utils import *
from keras import backend as K

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
    denom = (ux**2+uy**2+c1)*(varx+vary+c2)
    return ssim/K.clip(denom, K.epsilon(), np.inf)

def optxloss(y_true, y_pred):
    def LG(arr):
        kernel = np.zeros((5, 5, 3, 3))
        kernel[0,0,:,:] = kernel[4,0,:,:] = kernel[0,4,:,:] = kernel[4,4,:,:] = 5
        kernel[0,1,:,:] = kernel[0,3,:,:] = kernel[1,0,:,:] = kernel[1,4,:,:] = 3
        kernel[3,0,:,:] = kernel[3,4,:,:] = kernel[4,1,:,:] = kernel[4,3,:,:] = 3
        kernel[0,2,:,:] = kernel[2,0,:,:] = kernel[2,4,:,:] = kernel[4,2,:,:] = 0
        kernel[1,1,:,:] = kernel[1,3,:,:] = kernel[3,1,:,:] = kernel[3,3,:,:] = -12
        kernel[1,2,:,:] = kernel[2,1,:,:] = kernel[2,3,:,:] = kernel[3,2,:,:] = -23
        kernel[2,2,:,:] = -40
        kernel = K.variable(kernel)
        edge = K.conv2d(arr, kernel, strides=(1, 1), padding='same', data_format='channels_last')
        mn = K.min(edge)
        mx = K.max(edge)
        return (edge-mn)/(mx-mn)
    return 0.7*K.mean(K.square(y_pred-y_true))+0.3*K.mean(K.square(LG(y_pred)-LG(y_true)))

def optxloss2(y_true, y_pred):
    def LG(arr):
        kernel = np.zeros((5, 5, 1, 1))
        kernel[0,2,:,:] = kernel[2,0,:,:] = kernel[2,4,:,:] = kernel[4,2,:,:] = 1
        kernel[1,1,:,:] = kernel[1,3,:,:] = kernel[3,1,:,:] = kernel[3,3,:,:] = 1
        kernel[1,2,:,:] = kernel[2,1,:,:] = kernel[2,3,:,:] = kernel[3,2,:,:] = 2
        kernel[2,2,:,:] = -16
        kernel = K.variable(kernel)
        return K.conv2d(arr, kernel, strides=(1, 1), padding='same', data_format='channels_last')
    return 0.7*K.mean(K.square(y_pred-y_true))+0.3*K.mean(K.square(LG(y_pred)-LG(y_true)))

name = ""

def denoise():
    files = [f for f in glob.glob('demo/*.png')]
    files.sort() #diffuse, noisy
    name = files[1][files[1].index('_')+1: files[1].index('.')]
    x = np.array([cv.imread(files[1])])/255.
    d = np.array([cv.imread(files[0])])/255.
    input = np.concatenate((x, d), axis = 3)
    model = load_model('ml/models/'+name+'_final.h5', custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
    return model.predict(input)[0]

def superres(img):
    pre = np.array([cv.split(cv.cvtColor(cv.resize(img, (1024, 1024)), cv.COLOR_BGR2YCrCb))])
    x = np.array([cv.resize(pre[0][0], (1024, 1024)).reshape(1024, 1024, 1)])
    model = load_model('ml/models/sr_final.h5', custom_objects={"optxloss": optxloss2, "PSNRLoss": PSNRLoss, "SSIM": SSIM})
    res = model.predict(x)
    temp = cv.merge(np.array([res[0].reshape((1024, 1024)), pre[0][1].reshape((1024, 1024)), pre[0][2].reshape((1024, 1024))]))
    return cv.cvtColor(temp, cv.COLOR_YCrCb2BGR)

img = denoise()
img = superres(img)
cv.imwrite('demo/res.png', img*255.)
