import math, time, string
from Tkinter import *

SCALEFONT = "-adobe-helvetica-bold-r-*-*-*-120-*-*-*-*-*-*"
DIGITFONT = "-*-lucidatypewriter-*-r-*-*-*-120-*-*-m-*-*-*"
TITLEFONT = "-adobe-helvetica-regular-r-*-*-*-120-*-*-*-*-*-*"

WINDOWPAD = 5

class Gauge :
    def __init__(self, parentcanvas,
                 x, y,                  # Origin
                 min         = 0,       # Minimum value
                 max         = 100,     # Maximum value
                 scalevalues = None,    # List, eg. [0, 50, 100]
                 scalearc    = 240,     # Angle of scale in degrees
                 minlocation = 240,     # Angle in degrees of min value
                 direction   = 0,       # Default clockwise; 1 for anti-
                 radius      = 65,      # Radius in pixels
                 handcolour  = "red",   # Colour of hand
                 ticklen     = 8,       # Length of ticks in pixels
                 bigticks    = None,    # Set big ticks when value % n == 0
                 bigticklen  = 12,      # Length of big ticks
                 unitspertick= 5,       # Units per tick
                 knobradius  = 4,       # Radius of knob in pixels
                 knobcolour  = "black", # Colour of knob
                 scalefont   = SCALEFONT,
                 scalecolour = "black",
                 scalesep    = 20,      # Separation of scale text from rim
                 digitformat = "%d",    # Format of digital display
                 digitfont   = DIGITFONT,
                 digitcolour = "black",
                 digitpos    = 35,      # Position of centre of digital display
                 title       = "",
                 titlefont   = TITLEFONT,
                 titlecolour = "black",
                 titlepos    = 25,      # Position of centre of title
                 linecolour  = "black", # Colour of lines
                 facecolour  = "white", # Colour of gauge face
                 bgcolour    = "blue"):

        if scalevalues is None:
            scalevalues = [min, (min + max) / 2, max]

        # Things we need to remember
        self.val           = None
        self.min           = min
        self.max           = max
        self.scalesep      = scalesep
        self.scalevalues   = scalevalues
        self.scalefont     = scalefont
        self.scalecolour   = scalecolour
        self.scalearc_r    = scalearc * math.pi / 180
        self.minlocation_r = minlocation * math.pi / 180
        self.direction     = direction
        self.radius        = radius
        self.handcolour    = handcolour
        self.knobradius    = knobradius
        self.digitformat   = digitformat
        self.digitfont     = digitfont
        self.digitcolour   = digitcolour
        self.digitpos      = digitpos
        self.titlefont     = titlefont
        self.titlecolour   = titlecolour
        self.titlepos      = titlepos

        # Polygon used to draw the hand
        self.handPolygon = [(-12,-12), (0,radius-5), (12,-12)]

        # Create a canvas for this gauge
        wwidth       = wheight = 2 * radius + 2 * WINDOWPAD
        tl           = WINDOWPAD
        br           = tl + radius * 2
        centre       = tl + radius
        self.centre  = centre
        self.gcanvas = Canvas(parentcanvas,
                              height=wheight, width=wwidth, bg=bgcolour,
                              borderwidth=0, highlightthickness=0)

        # Draw the gauge and knob
        self.gcanvas.create_oval(tl,tl, br,br,
                                 fill=facecolour, outline=linecolour,
                                 width=3, tags="face")
        self.gcanvas.create_oval(centre - knobradius, centre - knobradius,
                                 centre + knobradius, centre + knobradius,
                                 fill=knobcolour)
        # Ticks
        tick = min
        while tick <= max:
            a  = self.angle(tick)
            sa =  math.sin(a)
            ca = -math.cos(a)

            if bigticks:
                if tick % bigticks == 0:
                    len = bigticklen
                else:
                    len = ticklen
            elif tick in scalevalues:
                len = bigticklen
            else:
                len = ticklen

            xof = (radius - len) * sa
            xot = radius         * sa
            yof = (radius - len) * ca
            yot = radius         * ca

            self.gcanvas.create_line(xof+centre, yof+centre,
                                     xot+centre, yot+centre,
                                     fill=linecolour, width=1)

            tick = tick + unitspertick

        # Scale
        self.drawScale()

        # Title
        self.gcanvas.create_text(centre, centre - titlepos,
                                 anchor=CENTER, fill=titlecolour,
                                 font=titlefont, text=title,
                                 justify=CENTER, tags="title")

        # Put gauge into parent canvas
        parentcanvas.create_window(x, y, width=wwidth, height=wheight,
                                   anchor=CENTER, window=self.gcanvas)

    def drawScale(self):
        scalerad = self.radius - self.scalesep
        centre   = self.centre
        for val in self.scalevalues:
            a = self.angle(val)

            self.gcanvas.create_text(centre + scalerad *  math.sin(a),
                                     centre + scalerad * -math.cos(a),
                                     anchor=CENTER, fill=self.scalecolour,
                                     font=self.scalefont, text=str(val))

    def set(self, value):
        """Set value of gauge"""

        if self.val == value:
            return

        self.val = value
        a        = self.angle(value)
        centre   = self.centre

        self.gcanvas.delete("hand")

        hp   = self.rotatePoints(self.handPolygon, a)
        hp   = self.translatePoints(hp, centre, centre)
        hl   = self.flattenPoints(hp)
        hand = self.gcanvas.create_polygon(hl, fill=self.handcolour,
                                           tags="hand")
        self.gcanvas.tag_raise(hand, "face")

        self.gcanvas.delete("digit")
        self.gcanvas.create_text(centre, centre + self.digitpos,
                                 anchor=CENTER, fill=self.digitcolour,
                                 font=self.digitfont,
                                 text=self.digitformat % value,
                                 tags="digit")

    def angle(self, value):
        """Convert value to an angle in radians"""

        if value < self.min:
            a = self.minlocation_r
        elif value > self.max:
            a = self.minlocation_r + self.scalearc_r
        else:
            a = self.minlocation_r + \
                self.scalearc_r * (value - self.min) / (self.max - self.min)

        while a > 2*math.pi:
            a = a - 2 * math.pi

        return a

    def rotatePoint(self, point, theta):
        """Rotate a point clockwise about the origin by an angle in radians"""

        x,y = point
        r   = math.sqrt(x*x + y*y)
        a   = math.atan2(y,x) - theta

        return (r * math.cos(a), r * -math.sin(a))

    def rotatePoints(self, points, theta):
        return map(lambda p, t=theta, rp=self.rotatePoint: rp(p, t), points)

    def translatePoints(self, points, x, y):
        return map(lambda p, x=x, y=y: (p[0]+x,p[1]+y), points)

    def flattenPoints(self, points):
        l = []
        for p in points:
            l.extend(list(p))
        return l



class Compass(Gauge):
    def __init__(self, parentcanvas,
                 x, y,                  # Origin
                 radius      = 65,      # Radius in pixels
                 handcolour  = "red",   # Colour of hand
                 ticklen     = 8,       # Length of ticks in pixels
                 bigticklen  = 12,      # Length of big ticks
                 knobradius  = 4,       # Radius of knob in pixels
                 knobcolour  = "black", # Colour of knob
                 scalefont   = SCALEFONT,
                 scalecolour = "black",
                 scalesep    = 20,      # Separation of scale text from rim
                 digitfont   = DIGITFONT,
                 digitcolour = "black",
                 digitpos    = 25,      # Position of centre of digital display
                 title       = "",
                 titlefont   = TITLEFONT,
                 titlecolour = "black",
                 titlepos    = 25,      # Position of centre of title
                 linecolour  = "black", # Colour of lines
                 facecolour  = "white", # Colour of gauge face
                 bgcolour    = "blue"):

        Gauge.__init__(self, parentcanvas, x, y,
                       min         = 0,          max          = 360,
                       minlocation = 0,          scalearc     = 360,
                       radius      = radius,     handcolour   = handcolour,
                       ticklen     = ticklen,    bigticks     = 45,
                       bigticklen  = bigticklen, unitspertick = 15,
                       knobradius  = knobradius, knobcolour   = knobcolour,
                       scalefont   = scalefont,  scalecolour  = scalecolour,
                       scalesep    = scalesep,   digitformat  = "%d deg",
                       digitfont   = digitfont,  digitcolour  = digitcolour,
                       digitpos    = digitpos,   title        = title,
                       titlefont   = titlefont,  titlecolour  = titlecolour,
                       titlepos    = titlepos,   linecolour   = linecolour,
                       facecolour  = facecolour, bgcolour     = bgcolour)

    def drawScale(self):
        scalerad = self.radius - self.scalesep
        centre   = self.centre
        for val,txt in [(0,"N"), (90,"E"), (180,"S"), (270,"W")]:
            a = self.angle(val)

            self.gcanvas.create_text(centre + scalerad *  math.sin(a),
                                     centre + scalerad * -math.cos(a),
                                     anchor=CENTER, fill=self.scalecolour,
                                     font=self.scalefont, text=txt)


class Clock(Gauge):
    def __init__(self, parentcanvas,
                 x, y,                  # Origin
                 radius      = 65,      # Radius in pixels
                 handcolour  = "red",   # Colour of hand
                 ticklen     = 8,       # Length of ticks in pixels
                 bigticklen  = 12,      # Length of big ticks
                 knobradius  = 4,       # Radius of knob in pixels
                 knobcolour  = "black", # Colour of knob
                 scalefont   = SCALEFONT,
                 scalecolour = "black",
                 scalesep    = 20,      # Separation of scale text from rim
                 digitfont   = DIGITFONT,
                 digitcolour = "black",
                 digitpos    = 25,      # Position of centre of digital display
                 title       = "",
                 titlefont   = TITLEFONT,
                 titlecolour = "black",
                 titlepos    = 25,      # Position of centre of title
                 linecolour  = "black", # Colour of lines
                 facecolour  = "white", # Colour of gauge face
                 bgcolour    = "blue"):

        Gauge.__init__(self, parentcanvas, x, y,
                       min         = 0,          max          = 60,
                       minlocation = 0,          scalearc     = 360,
                       radius      = radius,     handcolour   = handcolour,
                       ticklen     = ticklen,    bigticks     = 15,
                       bigticklen  = bigticklen, unitspertick = 5,
                       knobradius  = knobradius, knobcolour   = knobcolour,
                       scalefont   = scalefont,  scalecolour  = scalecolour,
                       scalesep    = scalesep,   digitformat  = None,
                       digitfont   = digitfont,  digitcolour  = digitcolour,
                       digitpos    = digitpos,   title        = "",
                       titlefont   = titlefont,  titlecolour  = titlecolour,
                       titlepos    = titlepos,   linecolour   = linecolour,
                       facecolour  = facecolour, bgcolour     = bgcolour)

        self.date = None

    def drawScale(self):
        scalerad = self.radius - self.scalesep
        centre   = self.centre
        for val,txt in [(0,"12"), (15,"3"), (30,"6"), (45,"9")]:
            a = self.angle(val)

            self.gcanvas.create_text(centre + scalerad *  math.sin(a),
                                     centre + scalerad * -math.cos(a),
                                     anchor=CENTER, fill=self.scalecolour,
                                     font=self.scalefont, text=txt)

    def set(self, value):
        """Set time. value is seconds since Unix epoch"""

        tt = time.gmtime(value)
        hm = string.lower(time.strftime("%I:%M %p", tt))
        if hm[0] == "0":
            hm = hm[1:]
        
        if self.val == hm:
            return

        self.val = hm

        hours   = tt[3] % 12
        mins    = tt[4]
        ha      = self.angle(hours * 5)
        ma      = self.angle(mins)
        mradius = self.radius - 5
        hradius = mradius * .75
        centre  = self.centre

        self.gcanvas.delete("hand")

        self.gcanvas.create_line(centre, centre,
                                 centre + hradius *  math.sin(ha),
                                 centre + hradius * -math.cos(ha),
                                 fill = self.handcolour, width = 4,
                                 arrow = "last", tags = "hand")
        self.gcanvas.create_line(centre, centre,
                                 centre + mradius *  math.sin(ma),
                                 centre + mradius * -math.cos(ma),
                                 fill = self.handcolour, width = 4,
                                 arrow = "last", tags = "hand")
        self.gcanvas.tag_raise("hand", "face")

        self.gcanvas.delete("digit")
        self.gcanvas.create_text(centre, centre + self.digitpos,
                                 anchor=CENTER, fill=self.digitcolour,
                                 font=self.digitfont,
                                 text=hm,
                                 tags="digit")

        date = time.strftime("%d %b %Y", tt)
        if date[0] == "0":
            date = date[1:]

        if self.date == date:
            return

        self.gcanvas.delete("title")
        self.gcanvas.create_text(centre, centre - self.titlepos,
                                 anchor=CENTER, fill=self.titlecolour,
                                 font=self.titlefont, text=date,
                                 justify=CENTER, tags="title")
