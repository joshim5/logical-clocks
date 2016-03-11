from multiprocessing import Process
from random import randint
from VirtualMachine import VirtualMachine
import time

def systemTimeGetter():
  return time.time()

if __name__ == "__main__":
  print "The Coordinator welcomes you."
  msgq_handler = [[],[],[]]
  ticks = [randint(1,6), randint(1,6), randint(1,6)]
  vm0 = VirtualMachine(ticks[0], msgq_handler, 0, [1,2], systemTimeGetter)
  vm1 = VirtualMachine(ticks[1], msgq_handler, 1, [0,2], systemTimeGetter)
  vm2 = VirtualMachine(ticks[2], msgq_handler, 2, [0,1], systemTimeGetter)

  # Start processes at same time
  p0 = Process(target=vm0.run)
  p1 = Process(target=vm1.run)
  p2 = Process(target=vm2.run)

  p0.start()
  p1.start()
  p2.start()

  # p0.terminate()
  # p1.terminate()
  # p2.terminate()

  p0.join()
  p1.join()
  p2.join()
