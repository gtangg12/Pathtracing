import numpy as np
import matplotlib.pyplot as plt

folder = 'DSPONZA'

def read(name):
    fin = open(folder+'/'+name, 'r')
    return np.array([float(line[:-1]) for line in fin.readlines()])

names = ['loss_history.txt', 'val_loss_history.txt', 'val_PSNR_history.txt', 'PSNR_history.txt']

tloss = read(names[0])
vloss = read(names[1])
tpsnr = 20.*np.full((100), np.log10(255))+read(names[2])
vpsnr = 20.*np.full((100), np.log10(255))+read(names[3])

x = np.arange(1, 101)

plt.subplot(2, 1, 1)
plt.plot(x, tloss)
plt.plot(x, vloss)
plt.ylabel('Loss')
#plt.legend(['Training', 'Validation'], loc='upper right')

plt.subplot(2, 1, 2)
plt.plot(x, tpsnr)
plt.plot(x, vpsnr)
plt.ylabel('PSNR')

plt.xlabel('sponza Epochs')
#plt.show()

plt.savefig('sponzaplot.png', bbox_inches='tight')
