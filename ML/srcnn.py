import numpy as np
import cv2 as cv
import glob
import time

N = 15000
raw_data = [file for file in glob.glob('train/*.jpg')]
y_train = raw_data[:N]
y_val = raw_data[N:]

from keras.models import *
from keras.callbacks import ModelCheckpoint
from keras.layers import *
from keras.layers.advanced_activations import *
from keras.optimizers import *
from keras.metrics import *
from keras.utils import *
from keras import backend as K

def PSNRLoss(y_true, y_pred):
    return -10.*K.log(K.mean(K.square((255.*(y_pred - y_true))))) / K.log(10.)

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

kernel = np.zeros((5, 5, 1, 1))
kernel[0,2,:,:] = kernel[2,0,:,:] = kernel[2,4,:,:] = kernel[4,2,:,:] = 1
kernel[1,1,:,:] = kernel[1,3,:,:] = kernel[3,1,:,:] = kernel[3,3,:,:] = 1
kernel[1,2,:,:] = kernel[2,1,:,:] = kernel[2,3,:,:] = kernel[3,2,:,:] = 2
kernel[2,2,:,:] = -16
kernel = K.variable(kernel)

def LG(arr):
    return K.conv2d(arr, kernel, strides=(1, 1), padding='same', data_format='channels_last')

def optxloss(y_true, y_pred):
    return 0.7*K.mean(K.square(y_pred-y_true))+0.3*K.mean(K.square(LG(y_pred)-LG(y_true)))

class Generator(Sequence):
    def __init__(self, y_set, batch_size):
        self.y_set = y_set
        self.batch_size = batch_size
    def __len__(self):
        return int(np.ceil(len(self.y_set)/float(self.batch_size)))
    def __getitem__(self, idx):
        by = self.y_set[idx*self.batch_size : (idx+1)*self.batch_size]
        y = np.array([cv.imread(file) for file in by])
        y = np.array([cv.split(cv.cvtColor(img, cv.COLOR_BGR2YCrCb))[0].reshape(img.shape[0], img.shape[1], 1) for img in y])
        x = np.array([cv.resize(cv.resize(img, (128, 128)), (256, 256)).reshape(img.shape[0], img.shape[1], 1) for img in y])
        #x = np.array([cv.resize(cv.resize(img, (128, 128)), (256, 256)) for img in y])
        return x/255., y/255.

def getModel():
    init = Input(shape=(None, None, 3))
    x = Conv2D(32, (9, 9), activation='relu', padding='same')(init)
    x = Conv2D(32, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(32, (5, 5), activation='relu', padding='same')(x)
    x = Conv2D(3, (3, 3), activation='relu', padding='same')(x)
    return Model(init, x)

def trainModel(model):
    checkpoint = ModelCheckpoint('SR/model-{epoch:03d}.h5', verbose=1, monitor='val_loss', save_best_only=True, mode='auto')
    model.compile(optimizer='adam', loss=optxloss, metrics=[PSNRLoss, SSIM])
    batch_size = 32
    trainGen = Generator(y_train, batch_size)
    valGen = Generator(y_val, batch_size)
    model.fit_generator(generator=trainGen,
                        steps_per_epoch=len(y_train)//batch_size,
                        epochs=1,
                        verbose=1,
                        validation_data=valGen,
                        validation_steps=len(y_val)//batch_size,
                        callbacks=[checkpoint],
                        shuffle=True)

    cv.imwrite('Test', temp[0])
    cv.waitKey(0)

def border(img, pos, color):
    temp = img.copy()
    w = 6
    cs = 256
    for i in range(pos[0], pos[0]+cs):
        for j in range(pos[1], pos[1]+cs):
            if i<pos[0]+w or i>pos[0]+cs-w-1 or j<pos[1]+w or j>pos[1]+cs-w-1:
                temp[i:i+1,j:j+1] = color
    return temp

def evalModel():
    #y = np.array([cv.imread(file) for file in raw_data][N+2500:])/255.
    #x = np.array([cv.resize(cv.resize(file, (128, 128)), (256, 256)) for file in y])
    #files = ['sponza241']
    filesx = [file for file in glob.glob('DATAC/*.jpg')]
    filesy = [file for file in glob.glob('DATAR/*.jpg')]
    pre0 = np.array([cv.resize(cv.imread(file), (1024, 1024)) for file in filesy])
    pre = np.array([cv.split(cv.cvtColor(img, cv.COLOR_BGR2YCrCb)) for img in pre0])
    y = np.array([cv.split(cv.cvtColor(img, cv.COLOR_BGR2YCrCb))[0].reshape(1024, 1024, 1) for img in pre0])
    #x = np.array([cv.resize(cv.resize(img, (256, 256)), (512, 512)).reshape(512, 512, 1) for img in y])
    x = np.array([cv.resize(cv.split(cv.cvtColor(cv.imread(file), cv.COLOR_BGR2YCrCb))[0], (1024, 1024)).reshape(1024, 1024, 1) for file in filesx])
    xtemp = np.array([cv.resize(img, (1024, 1024)) for img in x])
    """
    for i in range(len(x)):
        cv.imwrite('Test', x[i])
        cv.imwrite('Ref', y[i])
        cv.waitKey(0)
    """
    model = load_model('SRModels/model-035.h5', custom_objects={"optxloss": optxloss, "PSNRLoss": PSNRLoss, "SSIM": SSIM})
    #print(model.summary())
    print("EVAL")
    #print(model.evaluate(x/255., y/255., verbose=1))
    #start = time.time()
    res = model.predict(x/255.)
    #print(time.time()-start)
    res = res*255
    color = np.array([0, 255, 255])
    pos = (50, 100)
    for i in range(len(x)):
        temp = cv.merge(np.array([res[i].reshape((1024, 1024)), pre[i][1].reshape((1024, 1024)), pre[i][2].reshape((1024, 1024))]).astype(np.uint8))
        print(K.eval(PSNRLoss(K.variable(pre0[i]/255.), K.variable(cv.cvtColor(temp, cv.COLOR_YCrCb2BGR)/255.))))
        print(np.sqrt(np.mean((np.array([cv.cvtColor(temp, cv.COLOR_YCrCb2BGR)/255.])-np.array([pre0[i]/255.]))**2)))
        cv.imwrite('DATA2/'+str(i)+'st.jpg', cv.resize(cv.cvtColor(temp, cv.COLOR_YCrCb2BGR), (192, 192)))
        cv.imwrite('DATA2/'+str(i)+'sr.jpg', cv.resize(pre0[i], (192, 192)))
        cv.waitKey(0)

def cutOff():
    #files = [file for file in glob.glob('SRDATA/ROOM/*.jpg')]
    y = np.array([cv.imread(file) for file in files[:1]])/255.
    x = np.array([cv.resize(cv.resize(img, (512, 512)), (1024, 1024)) for img in y])
    #y = np.array([cv.imread(file) for file in raw_data][17500:17750])/255.
    #x = np.array([cv.resize(cv.resize(file, (128, 128)), (256, 256)) for file in y])
    modelsf = [file for file in glob.glob('SRM/*.h5')]
    modelsf.sort()
    print("HI")
    fout = open('SRM/ROOM_test_PSNR_history.txt', 'w')
    for file in modelsf:
        model = load_model(file, custom_objects={"PSNRLoss": PSNRLoss, "optxloss":optxloss, "SSIM":SSIM})
        print(file)
        temp = model.evaluate(x, y, verbose=1)
        print(temp[1])
        fout.write(str(temp[1]))

def testModel():
    pre0 = np.array([cv.imread(file) for file in raw_data][N+2500:])
    pre = np.array([cv.split(cv.cvtColor(img, cv.COLOR_BGR2YCrCb)) for img in pre0])
    y = np.array([cv.split(cv.cvtColor(img, cv.COLOR_BGR2YCrCb))[0].reshape(img.shape[0], img.shape[1], 1) for img in pre0])
    x = np.array([cv.resize(cv.resize(img, (128, 128)), (256, 256)).reshape(img.shape[0], img.shape[1], 1) for img in y])
    print("EVAL")
    model = load_model('SRModels/model-035.h5', custom_objects={"optxloss": optxloss, "PSNRLoss": PSNRLoss, "SSIM": SSIM})
    print(model.summary())
    print(model.evaluate(x/255., y/255., verbose=1))
    res = model.predict(x/255.)
    print(np.sqrt(np.mean((y/255. - res)**2)))

#trainModel(getModel())
evalModel()
#cutOff()
#testModel()
