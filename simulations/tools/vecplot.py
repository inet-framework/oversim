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
// Authors: Stephan Krause
//
"""

import matplotlib.pyplot as plt
import numpy as np
import scipy.stats as sp
from optparse import OptionParser 
from matplotlib.font_manager import fontManager, FontProperties
from string import maketrans
import re

def sortedDictValues(adict):
    keys = adict.keys()
    keys.sort()
    return map(adict.get, keys)

parser = OptionParser(usage="%prog [options] yvectorname *.vec")
parser.add_option("-i", "--include-only", type="string", dest="include", action="append", help="Include only iterations that match the regexp given by INCLUDE")
parser.add_option("-e", "--exclude", type="string", action="append", help="Exclude iterations that match the regexp given by EXCLUDE")
parser.add_option("-b", "--bucketsize", type="float", default=1.0, dest="bucketsize", help="Size of the buckets for vector aggregation to BUCKETSIZE (default=1.0)")
parser.add_option("-f", "--fill", action="store_true", default=False, help="Fill unset buckets with the value of the previous bucket")
parser.add_option("-d", "--fill-default", type="float", dest="fdef", help="Fill buckets before first entry with FILL_DEFAULT. This options implies -f.")
parser.add_option("-c", "--confidence-intervall", type="float", default=0, dest="ci", help="Plots the confidence intervals with confidence level LEVEL", metavar="LEVEL")
parser.add_option("-l", "--legend-position", type="int", default=0, dest="lpos", help="Position of the legend (default:auto)")
parser.add_option("-s", "--scale", type="float", default=1, help="Scale y values by a factor of SCALE")
parser.add_option("-r", "--range", type="int", nargs=2, metavar="START END", help="Restrict x axis to buckets in the range (START:END)")
parser.add_option("-o", "--outfile", help="Instead of displaying the plot, write a gnuplot-readable file to OUTFILE.dat and a gnuplot script to OUTFILE.plot. Requires --range option.")
(options, args) = parser.parse_args()

if len(args) < 2: 
    parser.error("Wrong number of options")

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

if options.outfile and not options.range:
    parser.error("Option 'outfile' only valid if --range is given")

if options.fdef:
    options.fill = True

valuemap = {}

# open all files
for infilename in args[1:]:
    inc = True
    if options.range:
        lastbucket=options.range[0]-1
    else:
        lastbucket=-1

    lastvalue = options.fdef

    infile = open(infilename, 'r')
    # read vector file
    parseHeader = True
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

            # find vector number
            if line.startswith("vector"):
                if line.find(args[0]) > -1:
                    vecnum = line.split(" ", 3)[1]
                    vars=confname + "-" + vars;
                    if vars not in valuemap:
                        valuemap[vars]={}
                    parseHeader = False
        else:
            # header parsing finished, search for the appropriate vectors
            splitline = line.split("\t")
            if splitline[0] == vecnum:
                # get value, and find the bucket for the data
                value = float(splitline[3])
                bucket = int(float(splitline[2])/options.bucketsize)

                # if range is given, stop if end of range is reached or ignore values before start of range
                if options.range:
                    if bucket > options.range[1]:
                        break
                    elif bucket < options.range[0]:
                        continue
                
                # if --fill was given, fill all buckets up to the current bucket with the last value
                if options.fill and bucket - lastbucket > 1 and lastvalue:
                    for i in range(lastbucket+1, bucket-1):
                        if i not in valuemap[vars]:
                            valuemap[vars][i]=[]
                        valuemap[vars][i].append(lastvalue)
                lastbucket = bucket
                lastvalue = value

                # insert value into bucket
                if bucket not in valuemap[vars]:
                    valuemap[vars][bucket]=[]
                valuemap[vars][bucket].append(value)

    # if range is given, fill all buckets up to the last
    if inc and options.fill and options.range and lastvalue and lastbucket < options.range[1]:
        for i in range(lastbucket+1, options.range[1]+1):
            if i not in valuemap[vars]:
                valuemap[vars][i]=[]
            valuemap[vars][i].append(lastvalue)

if len(valuemap) == 0:
    print "Error: vector \"" + args[0] + "\" not found, or all runs ignored due to the given include and exclude regexp!"
    exit(-1)

# Prepare plot
fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_xlabel("Time")
ax.set_ylabel(args[0])
ax.grid(True)

if options.outfile:
    outarray = np.arange(options.range[0], options.range[1]+1)

# Go through all runs
for run in sorted(valuemap.keys()):
    rundata = valuemap[run]
    row={}
    ci={}

    if options.outfile:
        if options.ci > 0:
            outcol = np.zeros((3, options.range[1] - options.range[0] + 1))
        else:
            outcol = np.zeros((1, options.range[1] - options.range[0] + 1))

    # Compute mean of all runs in all buckets
    for bucket, bucketdata in rundata.iteritems():
        bucketarray = np.array(bucketdata)
        bucketmean = bucketarray.mean()
        # determine coinfidence interval if desired
        if options.ci > 0:
            bucketci = sp.sem(bucketarray) * sp.t._ppf((1+options.ci)/2., len(bucketarray)) * options.scale
            ci[bucket] = bucketci
        bucketmean*=options.scale
        row[bucket] = bucketmean

        if options.outfile:
            outcol[0, bucket - options.range[0]] = bucketmean
            if options.ci > 0:
                outcol[1, bucket - options.range[0]] = bucketmean - bucketci
                outcol[2, bucket - options.range[0]] = bucketmean + bucketci
        
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
    outfile.write("#Bucket")
    for run in sorted(valuemap.keys()):
        outfile.write("," + run)
    outfile.write("\n")
    np.savetxt(outfile, outarray.T, delimiter=',')
    outfile.close()
    # Write gnuplot script
    outfile = open(options.outfile+".plot", "w")
    outfile.write("set terminal postscript eps\n")
    if options.ci > 0:
        outfile.write("set style data errorlines\n")
    else:
        outfile.write("set style data lines\n")
    outfile.write("set style line 2 pt 4\n")
    outfile.write("set style line 3 pt 3\n")
    outfile.write("set xlabel \"Time\"\n\n")
    outfile.write("set datafile separator \",\"\n")
    outfile.write("set ylabel \""+args[0]+"\"\n")
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

