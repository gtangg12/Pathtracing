import numpy as np
import cv2 as cv
import glob
import time

N = 7500
M = 1370
name = 'CBOX'
noisy0f = [file for file in glob.glob('Combined/'+name+'4ssp/*.jpg')]
cleanf = [file for file in glob.glob('Combined/'+name+'4096ssp/*.jpg')]
diff = [file for file in glob.glob('Combined/'+name+'dif/*.jpg')]
noisyf = [(noisy0f[i], diff[i]) for i in range(len(noisy0f))]
"""
ind = np.arange(len(cleanf))
np.random.seed(1)
np.random.shuffle(ind)
for val in range(len(ind)):
    noisyf[val], noisyf[ind[val]] = noisyf[ind[val]], noisyf[val]
    cleanf[val], cleanf[ind[val]] = cleanf[ind[val]], cleanf[val]
"""
"""
noisy0f = np.array(noisy0f[13:16])
cleanf = np.array(cleanf[13:16])
diff = np.array(diff[13:16])
noisyf = np.array(noisyf[13:16])
"""
"""
x_train = noisyf[:N]
y_train = cleanf[:N]
x_val = noisyf[N:N+M]
y_val = cleanf[N:N+M]
"""
from keras.models import *
from keras.callbacks import ModelCheckpoint
from keras.layers import *
from keras.layers.advanced_activations import *
from keras.optimizers import *
from keras.metrics import *
from keras.utils import *
from keras import backend as K

class Generator(Sequence):
    def __init__(self, x_set, y_set, batch_size):
        self.x_set = x_set
        self.y_set = y_set
        self.batch_size = batch_size
    def __len__(self):
        return int(np.ceil(len(self.x_set)/float(self.batch_size)))
    def __getitem__(self, idx):
        bx = self.x_set[idx*self.batch_size : (idx+1)*self.batch_size]
        by = self.y_set[idx*self.batch_size : (idx+1)*self.batch_size]
        x0 = np.array([cv.imread(p[0]) for p in bx])
        x1 = np.array([cv.imread(p[1]) for p in bx])
        x = np.concatenate((x0, x1), axis = 3)
        y = np.array([cv.imread(file) for file in by])
        return x/255., y/255.

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

def getModel():
    #encoder
    init = Input(shape=(None, None, 6))
    c1 = Conv2D(32, (3, 3), activation='relu', padding='same')(init)
    x = MaxPooling2D((2, 2), padding='same')(c1)
    c2 = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), padding='same')(c2)
    c3 = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), padding='same')(c3)
    c4 = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), padding='same')(c4)
    c5 = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), padding='same')(c5)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)

    #decoder
    x = UpSampling2D((2, 2))(x)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Add()([x, c5])
    x = UpSampling2D((2, 2))(x)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Add()([x, c4])
    x = UpSampling2D((2, 2))(x)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Add()([x, c3])
    x = UpSampling2D((2, 2))(x)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Add()([x, c2])
    x = UpSampling2D((2, 2))(x)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Add()([x, c1])
    x = Conv2D(3, (3, 3), activation='relu', padding='same')(x)

    return Model(init, x)

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

def trainModel(model):
    checkpoint = ModelCheckpoint('D'+name+'/model-{epoch:03d}.h5', verbose=1, monitor='val_loss', save_best_only=True, mode='auto')
    model.compile(optimizer='adam', loss=optxloss, metrics=[PSNRLoss, SSIM])
    batch_size = 32
    trainGen = Generator(x_train, y_train, batch_size)
    valGen = Generator(x_val, y_val, batch_size)
    history_callback = model.fit_generator(generator=trainGen,
                                           steps_per_epoch=len(y_train)//batch_size,
                                           epochs=1,
                                           verbose=1,
                                           validation_data=valGen,
                                           validation_steps=len(y_val)//batch_size,
                                           callbacks=[checkpoint],
                                           shuffle=True)
    loss_history = history_callback.history["loss"]
    numpy_loss_history = np.array(loss_history)
    np.savetxt('D'+name+"/loss_history.txt", numpy_loss_history, delimiter=",")
    loss_history = history_callback.history["val_loss"]
    numpy_loss_history = np.array(loss_history)
    np.savetxt('D'+name+"/val_loss_history.txt", numpy_loss_history, delimiter=",")
    loss_history = history_callback.history["PSNRLoss"]
    numpy_loss_history = np.array(loss_history)
    np.savetxt('D'+name+"/PSNR_history.txt", numpy_loss_history, delimiter=",")

def evalModel():
    """
    CBOX: 94
    CAR: 91
    ROOM: 78
    SPONZA
    """
    x = np.array([cv.imread(p[0]) for p in noisyf[:4]])/255.
    d = np.array([cv.imread(p[1]) for p in noisyf[:4]])/255.
    y = np.array([cv.imread(file) for file in cleanf[:4]])/255.
    """
    x = np.array([cv.imread('ROOM/ROOM4ssp/room256.jpg')])/255.
    d = np.array([cv.imread('ROOM/ROOMdif/room256.jpg')])/255.
    y = np.array([cv.imread('ROOM/ROOM4096ssp/room256.jpg')])/255.
    """
    model = load_model('DCBOX/model-094.h5', custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
    #model2 = load_model('DCBOXMSE/model-099.h5', custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
    #print(model.summary())
    #x = np.array([cv.imread(p[0]) for p in noisyf[N+M:N+M+50]])/255.
    #d = np.array([cv.imread(p[1]) for p in noisyf[N+M:N+M+50]])/255.
    #y = np.array([cv.imread(file) for file in cleanf[N+M:N+M+50]])/255.
    input = np.concatenate((x, d), axis = 3)
    res = model.predict(input)
    """
    cv.imshow('Test0', res[0])
    cv.imwrite('seval.jpg', cv.resize(255.*res[0], (256, 256)))
    img0 = res[0]
    x = np.array([x[0,:128], x[0,128:256], x[0,256:384], x[0,384:]])
    y = np.array([y[0,:128], y[0,128:256], y[0,256:384], y[0,384:]])
    d = np.array([d[0,:128], d[0,128:256], d[0,256:384], d[0,384:]])
    print(x.shape)
    print("EVAL")

    input = np.concatenate((x, d), axis = 3)
    start = time.time()
    #print(model.evaluate(input, y, verbose=1))
    res = model.predict(input)
    print(time.time()-start)
    #res2 = model2.predict(input)
    #print(K.eval(PSNRLoss(LG(LG(K.variable(y))), LG(LG(K.variable(res))))))
    #print(np.sqrt(np.mean((y-res)**2)
    img = np.zeros((512, 512, 3))
    img[:128] = res[0]
    img[128:256] = res[1]
    img[256:384] = res[2]
    img[384:] = res[3]
    cv.imshow('Test', img)
    cv.imwrite('peval.jpg', cv.resize(255.*img, (256, 256)))
    print(np.sqrt(np.mean((np.array(img0)-np.array(img))**2)))
    cv.waitKey(0)
    """
    for i in range(len(x)):
        cv.imshow('A', cv.resize(x[i], (256, 256)))
        cv.imshow('B', cv.resize(res[i], (256, 256)))
        cv.imshow('C', cv.resize(y[i], (256, 256)))
        cv.imwrite('DATAC/'+name+str(i)+'.jpg', 255*res[i])

def cutOff():
    x = np.array([cv.imread(p[0]) for p in noisyf[N+M:]])/255.
    d = np.array([cv.imread(p[1]) for p in noisyf[N+M:]])/255.
    y = np.array([cv.imread(file) for file in cleanf[N+M:]])/255.
    input = np.concatenate((x, d), axis = 3)
    modelsf = [file for file in glob.glob('D'+name+'/*.h5')]
    modelsf.sort()
    fout = open('D'+name+'/test_PSNR_history.txt', 'w')
    for file in modelsf:
        model = load_model(file, custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
        print(file)
        temp = model.evaluate(input, y, verbose=1)
        print(temp[1])
        fout.write(str(temp[1]))

#trainModel(getModel())
evalModel()
#cutOff()
