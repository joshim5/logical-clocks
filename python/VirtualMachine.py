from random import randint
import time
import os

class VirtualMachine(object):
  # Initialize with ticks, msgq_handler, msgq_id, and all_msgq_ids
  def __init__(self, ticks, msgq_handler, msgq_self_id, msgq_remainder_ids, system_time_getter):
    # Store parameters
    self.ticks = ticks
    self.msgq_handler = msgq_handler
    self.msgq_self_id = msgq_self_id
    self.system_time_getter = system_time_getter

    # Assign VM message queues
    vm0_id = randint(0,1)
    vm1_id = 1 - vm0_id

    self.msgq_vm0 = msgq_handler[msgq_remainder_ids[vm0_id]]
    self.msgq_vm1 = msgq_handler[msgq_remainder_ids[vm1_id]]
    self.msgq_self = msgq_handler[msgq_self_id]

    # Continuous listening
    self.keep_running = True

  # Log to message file
  def log(self, message):
    # Logging
    filename = str(self.msgq_self_id) + '.log'
    path = os.path.join('VM_logs', filename)
    with open(path, 'a') as log_file:
      log_file.write(message)

  def wipeLog(self):
    filename = str(self.msgq_self_id) + '.log'
    path = os.path.join('VM_logs', filename)
    open(path, 'w').close()

  # Local time - discrete units
  def getLocalTime(self):
    return self.local_time

  # System time - continuous units
  def getSystemTime(self):
    return self.system_time_getter()

  # Coordinates the logical clock
  def run(self):
    self.wipeLog()
    message = "Started VM {0} at system time {1}\n\n".format(self.msgq_self_id, self.getSystemTime())
    self.log(message)
    # VM's logical clock time
    self.local_time = 0

    # t0 for the system - updated in run()
    self.global_start_time = self.getSystemTime()

    while self.keep_running:
      self.tick()
      time.sleep(1./self.ticks)
      self.local_time += 1

  # Stop the VM from running
  def stop(self):
    self.keep_running = False
    self.log_file.close()

  def messageMachines(self, machine_qs):
    for machine_q in machine_qs:
      # Send the message
      message = "It is {0} o'clock on machine {1}.".format(self.getLocalTime(), self.msgq_self_id)
      machine_q.append(message)

    # Log this action
    log_message = "MESSAGE SENT:\t{0}\nSYSTEM TIME:\t{1}\nLOGICAL TIME:\t{2}\n\n\n".format(message, self.getSystemTime(), self.getLocalTime())
    self.log(log_message)

  def internalEvent(self):
    log_message = "INTERNAL EVENT\nSYSTEM TIME:\t{0}\nLOGICAL TIME:\t{1}\n\n\n".format(self.getSystemTime(), self.getLocalTime())
    self.log(log_message)

  # What to do at each time point
  def tick(self):
    choice = randint(1, 10)
    if choice == 1:
      machine_qs = [self.msgq_vm0]
      self.messageMachines(machine_qs)
    elif choice == 2:
      machine_qs = [self.msgq_vm1]
      self.messageMachines(machine_qs)
    elif choice == 3:
      machine_qs = [self.msgq_vm0, self.msgq_vm1]
      self.messageMachines(machine_qs)
    else:
      self.internalEvent()
