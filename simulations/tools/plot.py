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
// Authors: Stephan Krause, Ingmar Baumgart
//
"""

import matplotlib.pyplot as plt
import numpy as np
import scipy.stats as stats
from optparse import OptionParser 
from matplotlib.font_manager import fontManager, FontProperties
from string import maketrans
import re

def sortedDictValues(adict):
    keys = adict.keys()
    keys.sort()
    return map(adict.get, keys)

parser = OptionParser(usage="%prog [options] xvarname yscalarname *.sca")
parser.add_option("-i", "--include-only", type="string", dest="include", action="append", help="Include only iterations that match the regexp given by INCLUDE")
parser.add_option("-e", "--exclude", type="string", action="append", help="Exclude iterations that match the regexp given by EXCLUDE")
parser.add_option("-c", "--confidence-intervall", type="float", default=0, dest="ci", help="Plots the confidence intervals with confidence level LEVEL", metavar="LEVEL")
parser.add_option("-l", "--legend-position", type="int", default=0, dest="lpos", help="Position of the legend (default:auto)")
parser.add_option("-s", "--scale", type="float", default=1, help="Scale y values by a factor of SCALE")
parser.add_option("-S", "--offset", type="float", default=0, help="Add OFFSET to y values")
parser.add_option("-d", "--divide-by", type="string", dest="divexp", help="Divide values of yscalar by the values of DIVSCALAR", metavar="DIVSCALAR")
parser.add_option("-o", "--outfile", help="Instead of displaying the plot, write a gnuplot-readable file to OUTFILE.dat and a gnuplot script to OUTFILE.plot.")
parser.add_option("-x", "--xlabel", type="string", dest="xlabel", help="Label for the x-axis")
parser.add_option("-y", "--ylabel", type="string", dest="ylabel", help="Label for the x-axis")
(options, args) = parser.parse_args()

if len(args) < 3: 
    parser.error("Wrong number of options")

varregexp = re.compile("(, )?" + args[0] + "=[0-9.]+")

yParRegex = re.compile('"'+args[1]+'"')

if options.divexp:
    divexp = re.compile('"'+options.divexp+'"')

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

valuemap = {}
bucketlist = []
matchedScalars = []

yscalarFound = True;
dscalarFound = True;

# open all files
for infilename in args[2:]:
    inc = True

    if not yscalarFound:
        print "Error: y scalar \"" + args[1] + "\" not found in " + oldInfile + "!"
        exit(-1)
    
    if options.divexp and not dscalarFound:
        print "Error: div scalar \"" + options.divexp + "\" not found in " + oldInfile + "!"
        exit(-1)
    
    infile = open(infilename, 'r')
    # read scalar file
    parseHeader = True
    xvarFound = False
    for line in infile:
        if parseHeader:
            # parse iterationvars
            if line.startswith("attr configname "):
                confname = line[16:].strip()
            if line.startswith("attr iterationvars "):
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
                vars = vars.translate(maketrans('',''), '$"')
                #remove xvar from vars
                vars = varregexp.sub("", vars)
                vars = vars.lstrip(" ,")
                vars = confname + "-" + vars;
                if vars not in valuemap:
                    valuemap[vars]={}

            # find xvar
            if line.startswith("attr " + args[0] + " "):
                xvarFound = True
                bucket = float(line.split(" ", 3)[2])
            
            if line == "\n":
                parseHeader = False
                yscalarFound = False
                dscalarFound = False
                div = 1
                appenddata = []
                oldInfile = infilename;
                if not confname or not vars:
                    print "Couldn't parse scalar header. Is this a valid .sca file?"
                    exit(-1)                    

        elif xvarFound:
            # header parsing finished, search for the appropriate scalar
            splitline = line.split(" ", 1)
            if splitline[0] != "scalar":
                continue
            splitline = splitline[1].split("\t")
            # if y scalar regexp matches the line
            yParMatch = yParRegex.match(splitline[1])
            if yParMatch:
                yscalarFound = True
                if yParMatch.group(0) not in matchedScalars:
                    matchedScalars.append(yParMatch.group(0))
                # get value
                value = float(splitline[2])

                # insert value into bucket
                if bucket not in valuemap[vars]:
                    valuemap[vars][bucket]=[]
                if bucket not in bucketlist:
                    bucketlist.append(bucket)
                appenddata.append(value)

            if options.divexp:
                dParMatch = divexp.match(splitline[1])
                if dParMatch:
                    if dscalarFound:
                        print "Error: div scalar " + options.divexp + " matched twice in file " + infilename
                        exit(-1)
                    dscalarFound = True
                    # get value
                    div = float(splitline[2])

        else:
            print "Error: x variable \"" + args[0] + "\" not found, or all runs ignored due to the given include and exclude regexp!"
            exit(-1)

    # apply div
    if inc:
        for vals in appenddata:
            valuemap[vars][bucket].append(vals/div)


print "Using the following scalars for the plot:"
for matchedScalar in matchedScalars:
    print matchedScalar

# Prepare plot
fig = plt.figure()
ax = fig.add_subplot(111)
if options.xlabel:
    ax.set_xlabel(options.xlabel)
else:
    ax.set_xlabel(args[0])
if options.ylabel:
    ax.set_ylabel(options.ylabel)
else:
    ax.set_ylabel(args[1])    
ax.grid(True)

if options.outfile:
    bucketlist.sort()
    outarray = np.array(bucketlist)

# Go through all runs
for run in sorted(valuemap.keys()):
    rundata = valuemap[run]
    row={}
    ci={}

    if options.outfile:
        if options.ci > 0:
            outcol = np.zeros((3, len(bucketlist)))
        else:
            outcol = np.zeros((1, len(bucketlist)))

    # Compute mean of all runs in all buckets
    for bucket, bucketdata in rundata.iteritems():
        bucketarray = np.array(bucketdata)
        bucketmean = bucketarray.mean()
        # determine coinfidence interval if desired
        if options.ci > 0:
            bucketci = stats.sem(bucketarray) * stats.t._ppf((1+options.ci)/2., len(bucketarray)) * options.scale
            ci[bucket] = bucketci
        bucketmean = bucketmean*options.scale + options.offset
        row[bucket] = bucketmean

        if options.outfile:
            outcol[0, bucketlist.index(bucket)] = bucketmean
            if options.ci > 0:
                outcol[1, bucketlist.index(bucket)] = bucketmean - bucketci
                outcol[2, bucketlist.index(bucket)] = bucketmean + bucketci
        
    if options.outfile:
        outarray = np.vstack((outarray, outcol))
    else:
        # Plot row
        if options.ci == 0:
            ax.plot(sorted(row.keys()), sortedDictValues(row), label=run, marker='+')
        else:
            ax.errorbar(sorted(row.keys()), sortedDictValues(row),  sortedDictValues(ci), label=run, ecolor='k', marker='o', barsabove=True)

if options.outfile:
    # Write data to gnuplot readable file
    outfile = open(options.outfile+".dat", "w")
    outfile.write("#"+args[0])
    for run in sorted(valuemap.keys()):
        outfile.write("," + run)
    outfile.write("\n")
    np.savetxt(outfile, outarray.T, delimiter=',')
    outfile.close()
    # Write gnuplot script
    outfile = open(options.outfile+".plot", "w")
    outfile.write("set terminal postscript enhanced color eps lw 5 \"Helvetica-Bold\" 20 solid\n")
    outfile.write("set encoding iso_8859_1\n")    
    outfile.write("set border 31 linewidth 0.3\n")
    outfile.write("set pointsize 2.5\n")
    outfile.write("set style line 1 pt 7\n")
    outfile.write("set style line 2 pt 5\n")
    outfile.write("set style line 3 pt 6\n")
    outfile.write("set style line 4 pt 11\n")
    outfile.write("set style line 5 pt 4 lc 9\n")
    outfile.write("set style line 6 pt 2 lc 8\n")
    outfile.write("set style line 7 pt 8 lc 5\n")
    outfile.write("set style increment user\n")
    outfile.write("set datafile separator \",\"\n")
    outfile.write("set terminal postscript eps\n")
    if options.ci > 0:
        outfile.write("set style data errorlines\n")
    else:
        outfile.write("set style data linespoints\n")
    if options.xlabel:
        outfile.write("set xlabel \""+options.xlabel+"\" font \"Helvetica-Bold,24\"\n")
    else:
        outfile.write("set xlabel \""+args[0]+"\"\n")            
    if options.ylabel:
        outfile.write("set ylabel \""+options.ylabel+"\" font \"Helvetica-Bold,24\"\n")
    else:
        outfile.write("set ylabel \""+args[1]+"\"\n")                
    outfile.write("set output \"" + options.outfile + ".eps\"\n")
    outfile.write("plot ")
    i=2
    for run in sorted(valuemap.keys()):
        if options.ci > 0:
            outfile.write("\"" + options.outfile+".dat\" using 1:"+str(i)+":"+str(i+1)+":"+str(i+2)+" title \""+run+"\",")
            i += 3
        else:
            outfile.write("\"" + options.outfile+".dat\" using 1:"+str(i)+" title \""+run+"\",")
            i+=1
    # remove trailing ","
    outfile.seek(-1,1)
    outfile.truncate()
    outfile.write("\n")

else:
    # Display plot
    font=FontProperties(size='small')
    leg=ax.legend(loc=options.lpos, shadow=True, prop=font)
    plt.show()

