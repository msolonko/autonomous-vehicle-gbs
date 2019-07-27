# import the necessary packages
import cv2

class ShapeDetector:
    def __init__(self):
        pass
 
    def detect(self, c, bc):
        # initialize the shape name and approximate the contour
        shape = False
        peri = cv2.arcLength(c, True)
        if bc is None: #if biggest contour has not been found yet
            if peri > 50:
                approx = cv2.approxPolyDP(c, 0.04 * peri, True)
                shape = len(approx) == 5 and cv2.isContourConvex(approx)
        else:
            if peri > cv2.arcLength(bc, True): #if greater than the biggest contour found so far
                approx = cv2.approxPolyDP(c, 0.04 * peri, True)
                shape = len(approx) == 5 and cv2.isContourConvex(approx)
        return shape