#!/usr/bin/python

"""
//
// Copyright (C) 2009 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Authors: Ingmar Baumgart, Stephan Krause
//          (convex hull code from David Eppstein)
//
"""

import matplotlib
import numpy as np
import scipy.stats as stats
from optparse import OptionParser 
from matplotlib.font_manager import fontManager, FontProperties
from matplotlib import rc
from string import maketrans
import re
from matplotlib.widgets import Slider, Button, RadioButtons, CheckButtons
import itertools

def sorted_alphanum( l ): 
    """ Sort the given iterable in the way that humans expect (idea by Jeff Atwood).""" 
    convert = lambda text: int(text) if text.isdigit() else text 
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key[0].get_label()) ] 
    return sorted(l, key = alphanum_key)
                

#colorcycler = itertools.cycle(['b', 'g', 'r', 'c', 'm', 'y', 'k']).next
colorcycler = itertools.cycle(['b', 'g', 'r', 'm', 'k']).next
#colorcycler = itertools.cycle(['black']).next
linecycler = itertools.cycle(['-','--', '-.' , ':']).next
#markercycler = itertools.cycle(['+', ',', '.', '1', '2', '3', '4', '<', '>', 'D', 'H', '^', '_', 'd', 'h', 'o', 'p', 's', 'v', 'x', '|']).next
markercycler = itertools.cycle(['<','^','+','o','s','x','v','D','>','d','1','h','H','p']).next

#rc("axes", labelsize = 16)
#rc("xtick", labelsize= 16)
#rc("ytick", labelsize= 16)
#rc("legend", fontsize = 16)

def replaceLabel(label):
    if label in labelDict:
        return labelDict[label]
    else:
        return label

# convex hull by David Eppstein, UC Irvine, 7 Mar 2002
def orientation(p,q,r):
    '''Return positive if p-q-r are clockwise, neg if ccw, zero if colinear.'''
    return (q[1]-p[1])*(r[0]-p[0]) - (q[0]-p[0])*(r[1]-p[1])

def hulls(Points):
    '''Graham scan to find upper and lower convex hulls of a set of 2d points.'''
    U = []
    L = []
    Points.sort()
    for p in Points:
        while len(U) > 1 and orientation(U[-2],U[-1],p) <= 0: U.pop()
        while len(L) > 1 and orientation(L[-2],L[-1],p) >= 0: L.pop()
        U.append(p)
        L.append(p)
    return U,L

def onpick(event):
    thisline = event.artist
    ind = event.ind
    print 'onpick points:', data['label'][ind[0]]

def plotHull(filter, text):
    text = replaceLabel(text)
    if not text:
        return None
    reg = re.compile(filter);
    P = [(data['x'][i], data['y'][i], data['xci'][i], data['yci'][i]) for i in xrange(len(data['x'])) if reg.match(data['label'][i])]
    if len(P) == 0:
        return None
    U, L = hulls(P)

    if options.upperHull:
        x, y, xci, yci = zip(*[U[i] for i in xrange(len(U)) if ((i == 0) or (U[i][1] >= U[i-1][1]))])
    else:
        x, y, xci, yci = zip(*[L[i] for i in xrange(len(L)) if ((i == 0) or (L[i][1] <= L[i-1][1]))])

    if options.ci == 0:
        return [ax.plot(x, y, label=text, linestyle=linecycler(), color=colorcycler(),
                       marker=markercycler(), lw=1.5, markersize=9.0, markeredgewidth=2.0)[0],xci,yci]
    else:
        return [ax.errorbar(x, y, yci, xci, label=text, linestyle=linecycler(), color=colorcycler(),
                       marker=markercycler(), lw=1.5, markersize=9.0, markeredgewidth=2.0)[0],xci,yci]

    
parser = OptionParser(usage="%prog [options] xscalarname yscalaname *.sca")
parser.add_option("-u", "--upper", dest="upperHull", action="store_true", default=False, help="Plot the upper part of the convex hull")
parser.add_option("-o", "--outfile", help="Instead of displaying the plot, write a gnuplot-readable file to OUTFILE.dat and a gnuplot script to OUTFILE.plot")
parser.add_option("-i", "--include-only", type="string", dest="include", action="append", help="Include only iterations that match the regexp given by INCLUDE")
parser.add_option("-e", "--exclude", type="string", action="append", help="Exclude iterations that match the regexp given by EXCLUDE")
parser.add_option("-r", "--repetitions", dest="repetitions", action="store_true", default=False, help="Don't aggregate repetitions")
parser.add_option("-l", "--legend-position", type="int", default=0, dest="lpos", help="Position of the legend (default:auto)")
parser.add_option("-n", "--no-scatterpoints", dest="noscatterpoints", action="store_true", default=False, help="Disable plotting of scatter points")
parser.add_option("-g", "--no-grid", dest="nogrid", action="store_true", default=False, help="Don't plot a grid")
parser.add_option("-a", "--all-hull", dest="hull", action="store_true", default=False, help="Plot the convex hull over all points")
parser.add_option("-v", "--itervars", dest="itervars", default="", help="Use -vvar1,var2,... to enabled convex hulls for these itervars")
parser.add_option("-f", "--configs", dest="configs", action="store_true", default=False, help="Plot a convex hull for every configuration")
parser.add_option("-d", "--dict-label", dest="dictList", default=[], action="append", help="Use with -d oldlabel=newlabel to replace configname labels (may be used multiple times)")
parser.add_option("-c", "--confidence-intervall", type="float", default=0, dest="ci", help="Plots the confidence intervals with confidence level LEVEL (work in progress)", metavar="LEVEL")
parser.add_option("-m", "--mono", dest="mono", action="store_true", default=False, help="Don't use colors for Gnuplot output ")
parser.add_option("-z", "--zoom", type="string", dest="zoom", help="Use -zx1,x2,y1,y2 to select cropping pane")
parser.add_option("-x", "--xlabel", type="string", dest="xlabel", help="Label for the x-axis")
parser.add_option("-y", "--ylabel", type="string", dest="ylabel", help="Label for the y-axis")
(options, args) = parser.parse_args()

if len(args) < 3: 
    parser.error("Wrong number of options")

if options.outfile:
    matplotlib.use('Agg')
import matplotlib.pyplot as plt

labelDict = dict([i.split('=',1) for i in options.dictList]) 

xParRegex = re.compile(args[0])
yParRegex = re.compile(args[1])

#Compute filter regexps
incregex = []
if options.include:
    for incopt in options.include:
        incregex.append(re.compile(incopt))

exregex = []
if options.exclude:
    for exopt in options.exclude:
        exregex.append(re.compile(exopt))
        
if options.ci >=1 or options.ci < 0:
    parser.error("Option 'confidence-intervall': LEVEL must be between 0 and 1")

if options.repetitions:
    iterv = "attr iterationvars2 "
else:
    iterv = "attr iterationvars "

valuemap = {}

# open all files
for infilename in args[2:]:
    inc = True

    infile = open(infilename, 'r')
    # read scalar file
    parseHeader = True
    for line in infile:
        if parseHeader:
            # parse iterationvars
            if line.startswith("attr configname "):
                confname = line[16:].strip()
            if line.startswith(iterv):
                vars = line[19:].strip()

                # filter runs: exclude runs that don't match incregex, or that do match exregexp
                for exp in incregex:
                    if not exp.search(vars):
                        inc = False
                for exp in exregex:
                    if exp.search(vars):
                        inc = False
                if not inc:
                    break
                #remove $ and " chars from the string
                if( vars != '""'):
                    vars = vars.translate(maketrans('', ''), '$-"\\')
                    vars = vars.replace(", ", "-");
                    vars = confname + "-" + vars;
                else:
                    vars = confname
                if vars not in valuemap:
                    valuemap[vars] = {}

            if line == "\n":
                parseHeader = False
                
        else:
            # header parsing finished, search for the appropriate scalars
            splitline = line.split("\t")
            for reg in [xParRegex, yParRegex]:
                if reg.search(splitline[1]):
                    # get value, and find the bucket for the data
                    value = float(splitline[2])
                    # insert value into bucket
                    if reg.pattern not in valuemap[vars]:
                        valuemap[vars][reg.pattern] = []
                    valuemap[vars][reg.pattern].append(value)

# Go through all runs
data = {}
data['x'] = []
data['y'] = []
data['xci'] = []
data['yci'] = []
data['label'] = []
for run in sorted(valuemap.keys()):
    rundata = valuemap[run]
    row = {}
    ci = {}

    # Compute mean of all runs in all buckets
    for bucket, bucketdata in rundata.iteritems():
        bucketarray = np.array(bucketdata)
        row[bucket] = bucketarray.mean()
        # determine coinfidence interval if desired
        if options.ci > 0:
            ci[bucket] = stats.sem(bucketarray) * stats.t._ppf((1+options.ci)/2., len(bucketarray))                            

    if xParRegex.pattern not in row:
        print "Error: x scalar \"" + args[0] + "\" not found, or all runs ignored due to the given include and exclude regexp!"
        exit(-1)

    if yParRegex.pattern not in row:
        print "Error: y scalar \"" + args[1] + "\" not found, or all runs ignored due to the given include and exclude regexp!"
        exit(-1)
        
    data['x'].append(row[xParRegex.pattern])
    data['y'].append(row[yParRegex.pattern])
    if options.ci > 0:
        data['xci'].append(ci[xParRegex.pattern])
        data['yci'].append(ci[yParRegex.pattern])
    else:
        data['xci'].append(0)
        data['yci'].append(0)

    data['label'].append(run)

# find all configurations and itervars
itervars = {}
itervarsenabled = {}
confignames = set()
splititeroptions = options.itervars.split(",")
for key in valuemap.keys():
    m=re.match(r"(\w+)(-\w+=[\w\.]+)*$", key)
    confignames.add(m.group(1))
    for i in re.finditer(r"(\w+)=([\w\.]+)",  key):                 
        if i.group(1) not in itervars:
            itervars[i.group(1)] = set()
        if i.group(1) not in itervarsenabled:
            if i.group(1) in splititeroptions:
                itervarsenabled[i.group(1)] = True
            else:
                itervarsenabled[i.group(1)] = False
        itervars[i.group(1)].add(i.group(2))
        
# Prepare plot
fig = plt.figure()
ax = fig.add_subplot(111)

if not options.xlabel:
    options.xlabel = xParRegex.pattern;

if not options.ylabel:
    options.ylabel = yParRegex.pattern;
    
ax.set_xlabel(options.xlabel)
ax.set_ylabel(options.ylabel)
ax.grid(not options.nogrid)

if not options.outfile:
    fig.subplots_adjust(left=0.3)

scatterPoints = ax.scatter(data['x'], data['y'], label='_nolabel_', picker=5)
fig.canvas.mpl_connect('pick_event', onpick)
scatterPoints.set_visible(not options.noscatterpoints)

# plot convex hull over all runs
allHull = plotHull("", "all")
allHull[0].set_visible(options.hull)

# plot hulls for each configuration
configHulls = []
for config in confignames:
    h = plotHull(config + r"(-\w+=[\w\.]+)*$", config)
    if h:
        h[0].set_visible(options.configs)
        configHulls.append(h)

# plot hulls for each itervar
iterHulls = {}
for key in itervars:
    iterHulls[key] = []
    for val in itervars[key]:
        filter = r"\w+(-\w+=[\w\.]+)*-" + key + "=" + val + r"(-\w+=[\w\.]+)*$"
        h = plotHull(filter, "%s=%s" % (key, val))
        if h:
            if itervarsenabled[key]:
                h[0].set_visible(True)
            else:
                h[0].set_visible(False)
            iterHulls[key].append(h)

def toggleHullVisibility(label):
    if label=='scatter': scatterPoints.set_visible(not scatterPoints.get_visible())
    elif label=='all': allHull[0].set_visible(not allHull.get_visible())
    elif label=='config':
        options.configs = not options.configs
        for h in configHulls:
            h[0].set_visible(not h[0].get_visible())
    elif label in iterHulls:
        itervarsenabled[label] = not itervarsenabled[label]
        for h in iterHulls[label]:
            h[0].set_visible(not h[0].get_visible())
    legLine = []
    legLabel = []
    for i in [allHull] + configHulls:
        if i[0].get_visible():
            legLine.append(i[0])
            legLabel.append(i[0].get_label())
    for i in iterHulls:
        for j in iterHulls[i]:
            if j[0].get_visible():
                legLine.append(j[0])
                legLabel.append(j[0].get_label())

    font=FontProperties(size='small')
    if (len(legLine) > 0):
        ax.legend(legLine, legLabel, loc=options.lpos, shadow=True, prop=font)
    else:
        ax.legend(("",),loc=options.lpos, shadow=True, prop=font)
    plt.draw()

if not options.outfile:
    rax = plt.axes([0.05, 0.15, 0.15, 0.6])
    check = CheckButtons(rax, ['scatter', 'all', 'config'] + itervars.keys(), (scatterPoints.get_visible(), allHull[0].get_visible(), options.configs) + tuple(itervarsenabled.values()))
    check.on_clicked(toggleHullVisibility)

toggleHullVisibility("")
if options.zoom:
    limits = options.zoom.split(",");
    if len(limits) != 4:
        print "-z parameter needs 4 comma-seperated float values!"
        exit(-1)
    ax.set_xlim(float(limits[0]), float(limits[1]))
    ax.set_ylim(float(limits[2]), float(limits[3]))

plt.show()

if options.outfile:
#    fig.savefig(options.outfile)
    # Write gnuplot script
    outfile = open(options.outfile+".plot", "w")
    if options.mono:
      outfile.write("set terminal postscript enhanced eps dl 3 lw 4 \"Helvetica-Bold\" 20\n")
    else:
      outfile.write("set terminal postscript enhanced color eps lw 5 \"Helvetica-Bold\" 20 solid\n")
    outfile.write("set border 31 linewidth 0.3\n")
    if options.ci == 0:
      outfile.write("set style data linespoints\n")
    else:
      outfile.write("set style data xyerrorlines\n")
    outfile.write("set pointsize 2.5\n")
    outfile.write("set style line 1 pt 7\n")
    outfile.write("set style line 2 pt 5\n")
    outfile.write("set style line 3 pt 6\n")
    outfile.write("set style line 4 pt 11\n")
    if options.mono:
      outfile.write("set style line 5 pt 4\n")
      outfile.write("set style line 6 pt 2\n")
      outfile.write("set style line 7 pt 8\n")
    else:
      outfile.write("set style line 5 pt 4 lc 9\n")
      outfile.write("set style line 6 pt 2 lc 8\n")
      outfile.write("set style line 7 pt 8 lc 5\n")
    outfile.write("set style increment user\n")
    outfile.write("set datafile separator \",\"\n")
    if options.xlabel:
        outfile.write("set xlabel \""+options.xlabel+"\" font \"Helvetica-Bold,24\"\n")
    if options.ylabel:
        outfile.write("set ylabel \""+options.ylabel+"\" font \"Helvetica-Bold,24\"\n")
    if options.zoom:
        outfile.write("set xrange ["+limits[0]+":"+limits[1]+"]\n")
        outfile.write("set yrange ["+limits[2]+":"+limits[3]+"]\n")
    outfile.write("set output \"" + options.outfile + ".eps\"\n")
    outfile.write("plot ")
    
    # Write data to gnuplot readable file
    datfile = open(options.outfile+".dat", "w")
    plotidx = 0
    for h in sorted_alphanum(configHulls):
        if h[0].get_visible():
            outfile.write("\"" + options.outfile+".dat\" index "+str(plotidx)+" title \""+h[0].get_label()+"\",")
            datfile.write("# " + h[0].get_label() + "\n")
            if options.ci == 0:
                np.savetxt(datfile, zip(h[0].get_xdata(), h[0].get_ydata()), delimiter=',')
            else:
              np.savetxt(datfile, zip(h[0].get_xdata(), h[0].get_ydata(), h[1], h[2]), delimiter=',')
            datfile.write("\n\n")
            plotidx += 1
    for i in iterHulls:
        for j in sorted_alphanum(iterHulls[i]):
            if j[0].get_visible():
                outfile.write("\"" + options.outfile+".dat\" index "+str(plotidx)+" title \""+j[0].get_label()+"\",")
                datfile.write("# " + j[0].get_label() + "\n")
                if options.ci == 0:
                    np.savetxt(datfile, zip(j[0].get_xdata(), j[0].get_ydata()), delimiter=',')
                else:
                    np.savetxt(datfile, zip(j[0].get_xdata(), j[0].get_ydata(), j[1], j[2]), delimiter=',')
                datfile.write("\n\n")
                plotidx += 1
    
    # remove trailing ","
    outfile.seek(-1,1)
    outfile.truncate()
    outfile.write("\n")
    
    outfile.close()
    datfile.close()


currentOptions = ""

if not scatterPoints.get_visible():
    currentOptions += "-n "
if allHull[0].get_visible():
    currentOptions += "-h "
if options.ci != 0:
    currentOptions += "-c%f " % (options.ci)    
if options.configs:
    currentOptions += "-f "

enabledVars = ""
for key in itervars:
    if itervarsenabled[key]:
        if enabledVars:
            enabledVars += ","
        enabledVars += "%s" % (key)

if enabledVars:
    currentOptions += "-v%s " % (enabledVars)
    
print "scatterplot %s-z%f,%f,%f,%f" % (currentOptions, ax.get_xlim()[0], ax.get_xlim()[1], ax.get_ylim()[0], ax.get_ylim()[1])
