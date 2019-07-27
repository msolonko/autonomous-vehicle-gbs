# import the necessary packages
from shapedetector import ShapeDetector
import argparse
import imutils
import cv2
import numpy as np
# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--video", 
    help="path to the input video")
args = vars(ap.parse_args())
lower = np.array([0, 164, 169], dtype = "uint8")
upper = np.array([138, 255, 255], dtype = "uint8")
sd = ShapeDetector()
if args["video"] is None:
    cap = cv2.VideoCapture(0)
else:
    cap = cv2.VideoCapture(args["video"])
f_count = 0
print("Forward(50)")
while(cap.isOpened()):
  # Capture frame-by-frame
  ret, frame = cap.read()
  f_count += 1
  if ret == True and f_count == 1: #adjust here for skipping frames
    f_count = 0  
    
    #resize frame    
    resized = imutils.resize(frame, width=640, height=480)
    
    #color mask
    mask = cv2.inRange(resized, lower, upper)

    cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
        cv2.CHAIN_APPROX_SIMPLE)
    cnts = imutils.grab_contours(cnts)
    
    #variable to store biggest contour
    biggest_c = None
    # loop over the contours
    for c in cnts:
        
        #if it fits criteria, update biggest_c variable with current contour
        if sd.detect(c, biggest_c):
            biggest_c = c
         
    if biggest_c is not None:#if contour found
        M = cv2.moments(biggest_c)
        cX = int(M["m10"] / M["m00"])
        cY = int(M["m01"] / M["m00"])
        cX_track = cX-320 #find difference in pixels from center
        angle = int(cX_track / 320 * 50) #-50 to 50 degrees steering. Might need to flip sign
        print("Steer("+str(angle)+")") #command to arduino
                    
        biggest_c = biggest_c.astype("int")
        #cv2.drawContours(resized, [biggest_c], -1, (255, 0, 0), 2)
        cv2.rectangle(resized, (cX - 5, cY - 5), (cX + 5, cY + 5), (255, 0, 0), -1) #draws center
        cv2.imshow("Image", resized)
 
    # Press Q on keyboard to  exit
    if cv2.waitKey(25) & 0xFF == ord('q'):
      break
 
  # Break the loop
  elif ret == False:
      break
  
# When everything done, release the video capture object
cap.release()
# Closes all the frames
cv2.destroyAllWindows()