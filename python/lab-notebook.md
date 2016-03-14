# Lab Notebook
This lab notebook tracks our progress, thoughts and reaction on the development of scale models. In particular, we are implementing these systems in order to better understand logical clocks. All observations are recorded here. Here, we consider Git to be our [corroborating witness](http://www.otc.umd.edu/inventors/lab-notebooks). We chose this Git approach, for it allows us to soft-delete observations, for clarity and organization. We can change the formatting of this document, but still maintain evidence of our initial train of thought. Furthermore, our commits serve as digital signatures and we can even cryptographically [sign our work](https://git-scm.com/book/en/v2/Git-Tools-Signing-Your-Work), if desired.

This is a living document.

## Initial Considerations

### Threading in Python

As we are developing virtual machines with heavy reliance on communication protocols, we first sought a thorough understanding of threading in our desired programming language implementations. While coding in Python would increase the rate of development, as compared to lower-level languages, we quickly observed that threading in most Python implementations is a touchy subject. In particular, some implementations of Python use a [Global Interpreter Lock](https://wiki.python.org/moin/GlobalInterpreterLock), which  is a mutex that prevents multiple native threads from executing Python bytecodes at once. This lock is necessary mainly because Python's memory management is not thread-safe, in many implementations.

The GIL is controversial because it prevents multi-threaded Python programs from taking full advantage of multiprocessor systems in certain situations. While this approach can be faster in the single-threaded case, our study requires true asynchrony, in order for our virtual machines to best model real [distributed systems](http://www.webopedia.com/TERM/V/virtual_machine.html).

### A Note on Denotations
Python does not have a GIL. Python is a programming language. A programming language is a set of abstract mathematical rules and restrictions ([Source: Lecture notes, CS152 by Prof. Stephen Chong](http://www.seas.harvard.edu/courses/cs152/2015sp/)). There is nothing in the Python Language Specification which says that there must be a GIL.

### Decisions
While we have less control of threading in Python, as compared to lower-level languages like Java and C, we were unsure whether this would impact the results of our experiments. We considered using non-standard implementations of forking, or finding someway around the GIL. Indeed, computations of some libraries, such as [NumPy](http://www.numpy.org/), happen _outside_ of the GIL, so this is a feasible possibility. We will return to this after initial code development is complete.

In order to check whether this will impact the results, we plan to conduct a controlled experiment! We will first implement the scale models in Python and later port to C after everything is working.

## Development Progress
### Object-Oriented Paradigms
We will write a `VirtualMachine` class, that is initialized with some ticks parameter, denoting the number of clock ticks per second for that machine. The class will have a method called `run()` that fires the listening processes. At each time unit for this machine, a method called `tick()` will be called.

### Alternative: Separate Files
Another design strategy is to run each Python script as a separate machine, and have them communicate by some socket or local data file. Indeed, this might overcome the GIL issue. We might return to this idea and experiment with this after the initial development.

### Communication
With the object-oriented paradigm, the easiest way to create asynchronous machines is to instantiate the objects separately, and have them communicate via shared lists in memory. Each list represents a virtual machine's message queue. A message queue handler (in our simple model, just a dictionary of lists, with some helper functions) will allow each virtual machine to access the message queues of other machines. Allowed operations:

* A machine can append messages to other machines' message queues.
* A machine can pop messages from its own queue.

We considered having this handler take control of the message sending infrastructure, but we decided that this should be maintained in the `VirtualMachine` class, since the virtual machines should decide which machines should get the messages.

### Running Threads Synchronously
After implementing the `VirtualMachine` class and starting the `Coordinator` file, we face a new issue. After initializing all the `VirtualMachine`s, how can we have each `run()` method be called at the same time?

A great [stackoverflow answer](http://stackoverflow.com/questions/2108126/how-to-run-two-functions-simultaneously) referred us to the `multiprocessing` package, included with Python ≥ 2.7. The multiprocessing package offers both local and remote concurrency, effectively side-stepping the Global Interpreter Lock by using subprocesses instead of threads. Due to this, the multiprocessing module allows the programmer to fully leverage multiple processors on a given machine. It runs on both Unix and Windows.

This seems to be working! We read the same Global System Time off the top of the logs when we call:
```python
message = "Started VM {0} at system time {1}\n\n".format(self.msgq_self_id, self.getSystemTime())
    self.log(message)
```

### Analyzing preliminary results
Indeed, this is working properly. The initial time printed is the same across VMs. So have therefore enabled true parallelism. The results are unsurprising, too. There is a delay (in terms of system clock time) between sending and receiving across processes that have different logical tick times.

## Requirement of network sockets
We found [this post](https://canvas.harvard.edu/courses/9563/discussion_topics/104698) on Canvas. Prof. Waldo says not to use global structs (which are not really doing - yay!). However, the Python multiprocessing library, as far as we understand, spawns/forks a separate process for each thread. Which is not putting the entire model machine in a single process, which is the assignment. Furthermore, the assignment seems to require network sockets, which he have not enabled.

In order to ensure that we have done this correctly, we have decided to push forward on the C implementation. There, we are running everything within the same process. In any case, one could argue that C is better suited for this assignment. As we have experienced several times already, Python abstracts too much without telling you, and that can lead to problems. When designing distributed systems, it is better to use lower-level languages so that logical flaws do not arise in the future.
