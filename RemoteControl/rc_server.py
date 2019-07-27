#! /usr/bin/python
import os.path
import os
import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web
#pySerial
import serial
import threading

import RPi.GPIO as GPIO
import time

#platooning
from shapedetector import ShapeDetector
import imutils
import cv2
import numpy as np

##################  env vars ##################
serial_device = '/dev/ttyACM0'
#Tornado server port
PORT = 8080
###############################################

#Tornado Folder Paths
settings = dict(
        template_path = os.path.join(os.path.dirname(__file__), "templates"),
        static_path = os.path.join(os.path.dirname(__file__), "static")
        )

dev = serial.Serial(serial_device, 115200, timeout=30)
def SendSerial(message):
  dev.write(message.encode())
  #dev.flush()
  
def handle_data(data):
    print(data)

def read_from_port(ser):
  while True:
    print("serial read thread starting")
    reading = ser.readline().decode()
    handle_data(reading)
    


#thread = threading.Thread(target=read_from_port, args=(dev,))
#thread.start()

# This serial code assumes the arduino firmware is listening for the following commands:
#   for driving: d{integer} with the integer representing the power (negative is backwards)
#   for steering: s{integer} with the integer representing the angle of the front wheel (negative is left)
#   

#set GPIO Pins
GPIO_FRONT_US = 4
GPIO_RIGHT_US = 27
GPIO.setmode(GPIO.BCM)  

def Forward(power):
  SendSerial('d'+str(int(power))+'x')

def Backward(power):
  SendSerial('d-'+str(int(power))+'x')
  
def Steer(angle):
  SendSerial('s'+str(int(angle))+'x')
  
def Beep():
  SendSerial('b')

def Stop():
  SendSerial('d0x')
  
def getDistance(pin):
    #uses cm
  
    GPIO.setup(pin, GPIO.OUT)
    
    GPIO.output(pin, False)
    time.sleep(2e-6)
    # set Trigger to HIGH
    GPIO.output(pin, True)
 
    # set Trigger after 5uS to LOW
    time.sleep(5e-6)
    GPIO.output(pin, False)
 
    StartTime = time.time()
    StopTime = time.time()
 
    GPIO.setup(pin, GPIO.IN)
    
    # save StartTime
    while GPIO.input(pin) == 0:
        StartTime = time.time()
 
    # save time of arrival
    while GPIO.input(pin) == 1:
        StopTime = time.time()
 
    # time difference between start and arrival
    TimeElapsed = StopTime - StartTime
    # multiply with the sonic speed (34300 cm/s)
    # and divide by 2, because there and back
    d = (TimeElapsed * 34300) / 2
    return round(d, 2)

def UltraSonicTask():
  # runs car for 10 seconds unless object comes in front of it
  print("Starting UltraSonicTask")
  starting_time = time.time()
  current_time = time.time()
  Forward(120)
  print("Car Moving Forward")
  while current_time - starting_time < 30.0:
    #print("Time elapsed: " + str(round(current_time-starting_time, 1)))
    d = getDistance(GPIO_FRONT_US)
    #print("Distance: " + str(d))
    if d < 70:
      Stop()
    else:
      Forward(70)
    time.sleep(0.05)
    current_time = time.time()
  Stop()
  print("UltraSonicTask Complete")

def alphaTurn():
    Forward(70)
    time.sleep(10.5)
    Steer(25)
    time.sleep(8)
    Steer(0)
    time.sleep(7)
    Stop()

def ParallelPark():
  print("Beginning ParallelPark")
  Forward(40)
  d = getDistance(GPIO_RIGHT_US)
  dCount = 0 # for consistency
  while dCount < 5:
    d = getDistance(GPIO_RIGHT_US)
    if d > 70:
      dCount += 1
    else:
      dCount = 0
    print(d)
    time.sleep(0.05)
  starting = time.time()
  print("Empty spot found")
  while d > 70:
    d = getDistance(GPIO_RIGHT_US)
    print(d)
    time.sleep(0.05)
  print("End of spot detected")
  Stop()
  time_taken = time.time() - starting
  print("TIME TAKEN")
  print(time_taken)
  Backward(40)
  print("Going back to ideal spot")
  time.sleep(round(time_taken/2, 1)+0.5)
  print("Car is ready for parking")
  Stop()
  time.sleep(2) #can be removed later
  SendSerial('p')
  time.sleep(15) #actuators
  time.sleep(3.5) #drive sideways
  Stop()
  print("Stopped")
  print("Parked!")

def Garage():
  print("Garage task started")
  cap = cv2.VideoCapture(0)
  f_count = 0
  Forward(40)
  while(cap.isOpened()):
    ret, frame = cap.read()
    f_count += 1
    if ret == True and f_count == 1:
      f_count = 0
      resized = imutils.resize(frame, width = 640, height = 480)
      light = round(np.mean(resized))
      print(light)
      if light < 120:
        time.sleep(4)
        Stop()
        SendSerial('l1x')
        Beep()
        time.sleep(5)
        Backward(60)
        time.sleep(6)
        Stop()
        break
  

def Platoon():
  lower = np.array([0, 164, 169], dtype = "uint8")
  upper = np.array([138, 255, 255], dtype = "uint8")
  sd = ShapeDetector()
  os.system("pkill uv4l")
  cap = cv2.VideoCapture(1)
  f_count = 0
  
  Forward(50) #Starting car here
  
  while(cap.isOpened()):
    if getattr(thread, "platooning", True) == False:
      break
    # Capture frame-by-frame
    ret, frame = cap.read()
    #frame=cv2.flip(frame, -1)
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
          cX_track = cX-320 #find difference in pixels from center
          peri = int(cv2.arcLength(biggest_c, True))
          if peri > 50:
            angle = int(cX_track / 320 * 50) #-50 to 50 degrees steering. Might need to flip sign
            #print("Steer("+str(angle)+")") #command to arduino
            if angle > -50 and angle < 50:
              Steer(int(angle))
              #print(int(angle))
              #STEERING
            
            speed = 70 * 2 * (90/peri) + 30
            if 0 < speed and 255 > speed:
              if speed < 65:
                  Stop()
              else:
                  Forward(int(speed))
                  print(int(speed))
          
    # Break the loop
    elif ret == False:
        break
    
    
  # When everything done, release the video capture object
  cap.release()
  Stop()

thread = threading.Thread(target=Platoon)

class MainHandler(tornado.web.RequestHandler):
  def get(self):
     print("[HTTP](MainHandler) User Connected from " + self.request.remote_ip)
     self.render("index.html")


class WSHandler(tornado.websocket.WebSocketHandler):
  drive = 0
  steer = 0
  beep = False
  headlight = False
  SendSerial('l0x')
  sprint = False
  
  def open(self):
    print('[WS](WSHandler) Connection with ' + self.request.remote_ip + ' opened.')
 
  def on_message(self, message):
    #print('[WS] Incoming message:', message)
    if message == "w1":
      if self.sprint:
        Forward(200)
      else:
        Forward(70)
    if message == "a1":
      Steer(-50)
    if message == "s1":
      if self.sprint:
        Backward(200)
      else:
        Backward(70)
    if message == "d1":
      Steer(50)
    if message == "w0":
      Stop()
    if message == "a0":
      Steer(0)
    if message == "s0":
      Stop()
    if message == "d0":
      Steer(0)
    if message == "b":
      Beep()
    if message == "_":
      self.sprint = True
    if message == "_0":
      self.sprint = False
    if message == "platoon":
      thread.start()
      #Platoon()
    if message == "!platoon":
      thread.platooning = False
      thread.join()
    if message == "ultrasonicTask":
      UltraSonicTask()
    if message == "parallel":
      ParallelPark()
    if message == "alphaPark":
      SendSerial('y')
    if message == "garage":
      Garage()
    if message == "alphaTurn":
      alphaTurn()
        #SendSerial('z')
    if message == "l":
      if self.headlight:
        self.headlight = False
        SendSerial('l0x')
      else:
        self.headlight = True
        SendSerial('l1x')

  def on_close(self):
    print('[WS](WSHandler) Connection with ' + self.request.remote_ip + ' closed.')


application = tornado.web.Application([
  (r'/', MainHandler),
  (r'/ws', WSHandler),
  ], **settings)


if __name__ == "__main__":
    try:
        http_server = tornado.httpserver.HTTPServer(application)
        http_server.listen(PORT)
        main_loop = tornado.ioloop.IOLoop.instance()

        #print("[main] UV4L server starting")
        #os.system("uv4l --driver raspicam --width 640 --height 480 --hflip --vflip --server-option --port=8081")
        print("[main] Tornado Server starting")
        main_loop.start()

    except:
        print("[main] Exception triggered - Tornado Server stopped.")
        print("[main] pkilling uv4l.")
        os.system("pkill uv4l")
