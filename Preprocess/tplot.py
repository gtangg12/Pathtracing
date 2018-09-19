import cv2 as cv
import numpy as np

fin = open('sheet.txt', 'r')
x, y = [], []
for line in fin.readlines():
    arr = line[:-1].split(' ')
    if arr[0] == 'vt':
        x.append(float(arr[1]));
        y.append(float(arr[2]));
img = np.ones((501, 501, 1), np.uint8)*255
for i in range(0, len(x)):
    img[int(500*x[i]+0.5)][int(500*y[i]+0.5)][0] = 0
cv.imshow('Texture', img)
cv.waitKey(0)
