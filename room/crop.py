import cv2 as cv
import sys

img = sys.argv[1]
temp = cv.imread(img+'.jpg', 1)
l, w = temp.shape[:2]
cl = (l-400)//2
cw = (w-400)//2
cv.imwrite(img+'.jpg', temp[cl:cl+400,cw:cw+400])
