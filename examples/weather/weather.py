#!/usr/bin/env python

weatherIOR = "IOR:0003003c0000001449444c3a77656174686572496e666f3a312e300000000001000000000000003400010000000000187777772e756b2e72657365617263682e6174742e636f6d00669800000000000c3519450765f2b96f00000001"

CORBAInterval  = 1
UpdateInterval = 1

import Tkinter
import sys, threading, time
import gauge

from omniORB import CORBA
import _GlobalIDL

stateLock = threading.Lock()

immediateState  = None
cumulativeState = None

class CORBAThread(threading.Thread):
    def __init__(self, argv):
        global weatherIOR, immediateState, cumulativeState

        threading.Thread.__init__(self)
        self.setDaemon(1)

        self.orb = CORBA.ORB_init(argv, CORBA.ORB_ID)

        self.verbose = 0
        for arg in argv[1:]:
            if arg == "-v":
                self.verbose = 1
            else:
                weatherIOR = argv[1]

        self.wio = self.orb.string_to_object(weatherIOR)

        print "Acquiring initial weather state..."
        immediateState  = self.wio.immediate()
        cumulativeState = self.wio.cumulative()
        print "...initial state acquired."

    def run(self):
        global cumulativeState, immediateState

        print "CORBA thread started"

        while 1:
            time.sleep(CORBAInterval)
            if self.verbose: print "CORBA calls...",
            sys.stdout.flush()

            before = time.time()
            new_immediateState  = self.wio.immediate()
            new_cumulativeState = self.wio.cumulative()
            after = time.time()

            if self.verbose:
                print "done in", int((after-before) * 1000000), "microseconds"

            stateLock.acquire()
            immediateState  = new_immediateState
            cumulativeState = new_cumulativeState
            stateLock.release()


class GUI:

    def __init__(self):

        self.root   = Tkinter.Tk()
        self.root.title("AT&T Rooftop Weather")
        self.canvas = Tkinter.Canvas(self.root, width=800, height=500,
                                     background="blue")
        self.canvas.pack()

        self.root.protocol("WM_DELETE_WINDOW", self.delete_window)

        # Make gauges
        self.windDir = gauge.Compass(self.canvas, 100, 100,
                                     title        = "Wind\ndirection",
                                     handcolour   = "green")

        self.wind    = gauge.Gauge(self.canvas, 250, 100,
                                   title        = "Wind speed",
                                   min          = 0,
                                   max          = 100,
                                   unitspertick = 5,
                                   bigticks     = 25,
                                   digitformat  = "%d knots",
                                   handcolour   = "orange")

        self.windAvg = gauge.Gauge(self.canvas, 400, 100,
                                   title        = "Wind speed\n(10 min agv.)",
                                   min          = 0,
                                   max          = 100,
                                   unitspertick = 5,
                                   bigticks     = 25,
                                   digitformat  = "%.1f knots",
                                   handcolour   = "orange")

        self.temp    = gauge.Gauge(self.canvas, 550, 100,
                                   title        = "Temperature",
                                   min          = -10,
                                   max          = 35,
                                   scalevalues  = [-10,0,10,20,30],
                                   unitspertick = 1,
                                   bigticks     = 5,
                                   digitformat  = "%.1f C",
                                   handcolour   = "red")

        self.dewpt   = gauge.Gauge(self.canvas, 700, 100,
                                   title        = "Dew point",
                                   min          = -10,
                                   max          = 35,
                                   scalevalues  = [-10,0,10,20,30],
                                   unitspertick = 1,
                                   bigticks     = 5,
                                   digitformat  = "%.1f C",
                                   handcolour   = "cyan")

        self.press   = gauge.Gauge(self.canvas, 175, 250,
                                   title        = "Pressure\nQNH",
                                   min          = 950,
                                   max          = 1050,
                                   unitspertick = 5,
                                   digitformat  = "%d mB",
                                   handcolour   = "purple")

        self.humid   = gauge.Gauge(self.canvas, 325, 250,
                                   title        = "Humidity",
                                   min          = 0,
                                   max          = 100,
                                   unitspertick = 5,
                                   bigticks     = 25,
                                   digitformat  = "%d %%",
                                   handcolour   = "green")

        self.sun     = gauge.Gauge(self.canvas, 475, 250,
                                   title        = "Sun since\nmidnight",
                                   min          = 0,
                                   max          = 24,
                                   unitspertick = 1,
                                   bigticks     = 6,
                                   digitformat  = "%.2f hours",
                                   handcolour   = "yellow")

        self.rain    = gauge.Gauge(self.canvas, 625, 250,
                                   title        = "Rain since\nmidnight",
                                   min          = 0,
                                   max          = 100,
                                   unitspertick = 5,
                                   bigticks     = 25,
                                   digitformat  = "%.1f mm",
                                   handcolour   = "blue")

        self.clock   = gauge.Clock(self.canvas, 400, 400)


    def go(self):
        self.update()
        self.root.mainloop()
        self.root.destroy()

    def delete_window(self):
        self.root.quit()

    def update(self):
        stateLock.acquire()

        self.windDir.set(immediateState.windDirection)
        self.wind.   set(immediateState.windSpeed)
        self.windAvg.set(immediateState.rollingWindSpeed)
        self.temp.   set(immediateState.temperature)
        self.dewpt.  set(immediateState.dewpoint)
        self.press.  set(immediateState.pressure)
        self.humid.  set(immediateState.humidity)
        self.sun.    set(cumulativeState.sunshine)
        self.rain.   set(cumulativeState.rainfall)
        self.clock.  set(cumulativeState.end)

        stateLock.release()

        self.root.after(UpdateInterval * 1000, self.update)


ct = CORBAThread(sys.argv)
ct.start()

gui = GUI()
gui.go()